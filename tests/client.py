import socket
from multiprocessing import Process

import api

class Client(Process):
    def __init__(self, addr, port, log=None, requests=None, auto_open=True):
        super().__init__()
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.connect((addr, port))
        self.count = 0
        self.requests = None
        if requests:
            self.requests = requests
        self.log = None
        if log:
            self.log = open(log, "w+")
        if auto_open:
            self.open()

    def send(self, req):
        self.count += 1
        if self.log:
            self.log.write("%d %s" % (self.count, req))
        self.socket.send(req.encode())

    def recv(self):
        rep = self.socket.recv(200).decode()
        if self.log:
            self.log.write(rep)
        return rep

    def open(self):
        self.send(api.build_open())
        status, self.id = api.parse_open(self.recv())
        assert status == 200

    def deposit(self, amount):
        self.send(api.build_deposit(amount, self.id))
        return api.parse_update(self.recv())

    def withdraw(self, amount):
        self.send(api.build_withdraw(amount, self.id))
        return api.parse_update(self.recv())

    def transfer(self, amount, dst_id):
        self.send(api.build_transfer(amount, self.id, dst_id))
        return api.parse_update(self.recv())

    def run(self):
        for req in self.requests:
            self.send(req)
            rep = self.recv()
            status = api.get_status(rep)
            if not status or status >= 300:
                raise RuntimeError(req + rep)
        self.log.flush()
