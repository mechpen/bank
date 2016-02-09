import os
import sys
import time
import shutil
import subprocess

no_server = False
if len(sys.argv) > 1 and sys.argv[1] == "no_server":
    no_server = True

root_dir = os.path.realpath(__file__).rsplit("/", 2)[0]

exe_path = root_dir + "/src/bank"
log_dir = root_dir + "/tests/logs/"
db_dir = root_dir + "/tests/db/"
addr = "127.0.0.1"
port = 7890

server_process = None

def start_server(remove_old=True, extra_args=None, log=None):
    global server_process

    if no_server:
        return

    if remove_old:
        shutil.rmtree(db_dir, ignore_errors=True)
    os.makedirs(db_dir, mode=0o700, exist_ok=True)
    os.makedirs(log_dir, mode=0o700, exist_ok=True)

    if not log:
        log = "/dev/null"
    log = open(log, 'w')

    cmd = [exe_path, '-r', db_dir, '-a', addr, '-p', str(port)]
    if extra_args:
        cmd += extra_args
    print(" ".join(cmd))
    server_process = subprocess.Popen(cmd, stdout=log, stderr=log)

    time.sleep(1)

def stop_server(clean_all=False):
    if no_server:
        return

    server_process.terminate()
    if clean_all:
        shutil.rmtree(db_dir, ignore_errors=True)

def save_db():
    dbfile = db_dir + "accdb"
    dbbackup = dbfile + ".backup"
    return shutil.copyfile(dbfile, dbbackup)

def restore_db(backup):
    dbfile = db_dir + "accdb"
    shutil.move(backup, dbfile)
