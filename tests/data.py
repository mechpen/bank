import os
import struct

import server

def get_last_lsn():
    with open(server.db_dir+"wal", "r+b") as f:
        fd = f.fileno()
        off = os.fstat(fd).st_size
        while True:
            off -= 64
            buf = os.pread(fd, 64, off)
            _, _, sid = struct.unpack("<QQQ", buf[:24])
            if sid != 0:
                break
    return off // 64

def get_synced_lsn():
    with open(server.db_dir+"accdb", "r+b") as f:
        fd = f.fileno()
        buf = os.pread(fd, 64, 0)
        _, synced_lsn = struct.unpack("<QQ", buf[:16])
    return synced_lsn

def get_max_id():
    with open(server.db_dir+"accdb", "r+b") as f:
        fd = f.fileno()
        buf = os.pread(fd, 64, 0)
        _, _, max_id = struct.unpack("<QQQ", buf[:24])
    return max_id

def assert_lsn_id(last_lsn, max_id):
    assert last_lsn == get_last_lsn()
    assert max_id == get_max_id()

if __name__ == "__main__":
    print(get_last_lsn())
    print(get_synced_lsn())
    print(get_max_id())
