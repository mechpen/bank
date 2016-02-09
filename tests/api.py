import re

def build_open():
    return "OPEN\n"

def build_deposit(amount, aid):
    return "DEPOSIT %lu TO %lu\n" % (amount, aid)

def build_withdraw(amount, aid):
    return "WITHDRAW %lu FROM %lu\n" % (amount, aid)

def build_transfer(amount, src_id, dst_id):
    return "TRANSFER %lu FROM %lu TO %lu\n" % (amount, src_id, dst_id)

REP = re.compile("(\d+) - [^\n]+\n")
OPEN_REP = re.compile("200 - ACCOUNT NUMBER (\d+)\n")
UPDATE_REP = re.compile("200 - OLD AMOUNT (\d+) NEW AMOUNT (\d+)\n")

#define SERVER_ERROR  "500 - SERVER ERROR\n"
#define REQUEST_ERROR "400 - BAD REQUEST\n"
#define INSUFF_ERROR  "410 - INSUFFICIENT MONEY\n"
#define OVERFL_ERROR  "420 - TOO MUCH MONEY\n"

def get_status(rep):
    mo = REP.match(rep)
    if not mo:
        return None
    return int(mo.group(1))

def parse_open(rep):
    status = get_status(rep)
    if status != 200:
        return status, None
    mo = OPEN_REP.match(rep)
    if not mo:
        return status, None
    return status, int(mo.group(1))

def parse_update(rep):
    status = get_status(rep)
    if status != 200:
        return status, None, None
    mo = UPDATE_REP.match(rep)
    if not mo:
        return status, None, None
    return status, int(mo.group(1)), int(mo.group(2))
