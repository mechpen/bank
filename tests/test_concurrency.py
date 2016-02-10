import time
import random

import api
import data
import server
from client import Client

random.seed(1)

def execute_plan(name, plan):
    client_log_base = server.log_dir + "con_client_%s.log" % name
    clients = []
    for i in range(len(plan)):
        client = Client(server.addr, server.port, requests=plan[i],
                        auto_open=False, log=client_log_base+"."+str(i+1))
        client.start()
        clients.append(client)
    for client in clients:
        client.join()

def gen_open_plan(num_procs, num_opens):
    plan = []
    for _ in range(num_procs):
        reqs = [api.build_open()] * num_opens
        plan.append(reqs)
    return plan

def assert_max_id(max_id):
    log = server.log_dir + "con_client.log.v"
    client = Client(server.addr, server.port, log=log)
    assert client.id == max_id + 1

def test_open(num_procs, num_opens):
    plan = gen_open_plan(num_procs, num_opens)
    execute_plan("open", plan)
    assert_max_id(num_procs * num_opens)
    return num_procs * num_opens + 1

def gen_transaction_plan(num_procs, num_trans, num_users):
    amount = 9000000000000000000  # about half max_int64
    for i in range(num_users):
        init_deposit(i+1, amount)
    users = [amount] * num_users
    plan = []
    for _ in range(num_procs):
        reqs = [gen_random_req(num_users, users) for _ in range(num_trans)]
        plan.append(reqs)
    return plan, users

def init_deposit(i, amount):
    log = server.log_dir + "con_client.log.v"
    client = Client(server.addr, server.port, auto_open=False, log=log)
    client.id = i
    _, _, new = client.deposit(amount)
    assert new == amount

def gen_random_req(num_users, users):
    src = 1 + random.randrange(num_users)
    while True:
        dst = 1 + random.randrange(num_users)
        if src != dst:
            break
    amount = random.randrange(100)
    action = random.randrange(3)
    if action == 0:
        req = api.build_deposit(amount, src)
        users[src-1] += amount
    elif action == 1:
        req = api.build_withdraw(amount, src)
        users[src-1] -= amount
    else:
        req = api.build_transfer(amount, src, dst)
        users[src-1] -= amount
        users[dst-1] += amount
    return req

def assert_user_amount(i, amount):
    log = server.log_dir + "con_client.log.v"
    client = Client(server.addr, server.port, auto_open=False, log=log)
    client.id = i
    _, _, new = client.deposit(0)
    assert new == amount

def test_transaction(num_procs, num_trans, num_users):
    plan, users = gen_transaction_plan(num_procs, num_trans, num_users)
    execute_plan('trans', plan)
    for i in range(num_users):
        assert_user_amount(i+1, users[i])
    return num_procs * num_trans + num_users * 2

def test_concurrency():
    num_procs = 100
    num_opens = 100
    num_trans = 1000
    num_users = 3

    log = server.log_dir + "con_server.log"
    server_args = ["-s", "4096", "-b", "1", "-d", "100", "-v"]
    server.start_server(extra_args=server_args, log=log)
    try:
        lsn = test_open(num_procs, num_opens)
        lsn += test_transaction(num_procs, num_trans, num_users)
        data.assert_lsn_id(lsn, num_procs*num_opens+1)
    finally:
        server.stop_server()

def test_performance():
    num_users = 100
    num_procs = 100
    num_trans = 1000

    log = server.log_dir + "con_server.log.1"
    server.start_server(log=log)
    try:
        start = time.time()
        lsn = test_open(1, num_users)
        lsn += test_transaction(num_procs, num_trans, num_users)
        data.assert_lsn_id(lsn, num_users+1)
        dur = time.time() - start
        print("%d transactions in %.2f seconds, averaging at %.2f tps" % (lsn, dur, lsn/dur))
    finally:
        server.stop_server()

test_concurrency()
test_performance()

print("OK")
