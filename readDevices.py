from datetime import datetime
import json
from dbUtils import searchDevices, writeDatabase, writeMinute
import requests as req

with open('settings.json', "r") as file:
        settings = json.load(file)
dbFolder = settings['dbFolder']
logPath = settings['dbFolder'] + '/' + settings['logName']
timeWrite = settings["writeOnDbInterval"]

if __name__ == "__main__":
    for typeD in [0,1]: #agua, energia
        devices = searchDevices(typeD)
        for addr in devices.keys():
            try:
                res = req.get('http://' + addr + '/data').json()
                writeMinute(devices[addr], addr, typeD, res)

            except:
                with open(logPath, "a") as fp:
                    fp.write(f"[{datetime.now()}] ERROR: Failed to read from {addr}.\n")
                    res = {}

            if int(datetime.now().strftime("%M"))%timeWrite == 0: #write on dB
                writeDatabase(devices[addr],addr,typeD,res)
        