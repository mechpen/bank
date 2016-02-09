import data
import server
from client import Client

def repeat_deposit(client, repeat, amount):
    for i in range(repeat):
        client.deposit(amount)

server_args = ["-s", "4096", "-v", "-d", "100"]

def test_durability(repeat):
    server_log_base = server.log_dir + "dur_server_%d.log" % repeat
    client_log_base = server.log_dir + "dur_client_%d.log" % repeat
    server.start_server(extra_args=server_args, log=server_log_base+".1")
    try:
        client = Client(server.addr, server.port, log=client_log_base+".1")
        repeat_deposit(client, repeat, 100)
        saved = server.save_db()
        repeat_deposit(client, repeat, 100)
        server.stop_server()
        client.log.close()

        server.restore_db(saved)
        server.start_server(extra_args=server_args, log=server_log_base+".2", remove_old=False)
        client = Client(server.addr, server.port, log=client_log_base+".2", auto_open=False)
        client.id = 1
        _, _, new = client.deposit(0)
        client.log.close()
        assert new == repeat * 200

        data.assert_lsn_id(2*repeat+2, 1)
    finally:
        server.stop_server()

test_durability(10)
test_durability(1000)
print("OK")
