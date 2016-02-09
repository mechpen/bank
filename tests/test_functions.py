import random
import string

import api
import server
from client import Client

def random_string(n):
    return ''.join(random.choice(string.printable) for _ in range(n))

def test_functions():
    log_base = server.log_dir + "fun_client.log"
    client1 = Client(server.addr, server.port, log=log_base+".1")
    client2 = Client(server.addr, server.port, log=log_base+".2")
    assert client1.id == 1
    assert client2.id == 2

    status, old, new = client1.deposit(200)
    assert status == 200
    assert old == 0
    assert new == 200

    status, old, new = client1.withdraw(50)
    assert status == 200
    assert old == 200
    assert new == 150

    status, old, new = client1.transfer(50, 2)
    assert status == 200
    assert old == 150
    assert new == 100

    status, old, new = client2.withdraw(50)
    assert status == 200
    assert old == 50
    assert new == 0

    status, old, new = client2.withdraw(-1)
    assert status == 400

    status, old, new = client2.withdraw(1)
    assert status == 410

    max_int64 = 0xFFFFFFFFFFFFFFFF
    status, old, new = client2.deposit(max_int64)
    assert status == 200
    assert old == 0
    assert new == max_int64

    status, old, new = client2.deposit(1)
    assert status == 420

    for x in [-1, 0, 3]:
        client2.id = 3
        status, old, new = client2.deposit(1)
        assert status == 400

    client2.send('bad request\n')
    status = api.get_status(client2.recv())
    assert status == 400

    client2.send(random_string(1024)+'\n')
    status = api.get_status(client2.recv())
    assert status == 400

server.start_server(log=server.log_dir+"fun_server.log")
try:
    test_functions()
    print("OK")
finally:
    server.stop_server()
