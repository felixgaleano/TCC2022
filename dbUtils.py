from datetime import datetime
import json
import os
import sqlite3

#load settings
with open('settings.json', "r") as file:
        settings = json.load(file)
dbFolder = settings['dbFolder']
dbPath = settings['dbFolder'] + '/' + settings['dbName']
logPath = settings['dbFolder'] + '/' + settings['logName']
timeWrite = settings["writeOnDbInterval"]


if not(os.path.exists(dbFolder)):
    os.mkdir(dbFolder)

conn = sqlite3.connect(dbPath)
c = conn.cursor()

def createDatabase():
    cmd = """ CREATE TABLE IF NOT EXISTS devices (  id integer PRIMARY KEY,
                                                        name text NOT NULL,
                                                        type integer); """
    c.execute(cmd)

    cmd = """ CREATE TABLE IF NOT EXISTS avgEnergy (    id integer,
                                                        P1 REAL,P2 REAL,P3 REAL,
                                                        S1 REAL,S2 REAL,S3 REAL,
                                                        FP1 REAL,FP2 REAL,FP3 REAL,
                                                        V1 REAL,V2 REAL,V3 REAL,
                                                        I1 REAL,I2 REAL,I3 REAL,
                                                        E1 REAL,E2 REAL,E3 REAL,
                                                        N integer);"""
    c.execute(cmd)
    cmd = "CREATE UNIQUE INDEX idx_avgEnergy_id ON avgEnergy(id);"
    c.execute(cmd)

    cmd = """CREATE TABLE IF NOT EXISTS lastWater ( id integer, lastMeasure REAL );"""
    c.execute(cmd)
    cmd = "CREATE UNIQUE INDEX idx_lastWater_id ON lastWater(id);"
    c.execute(cmd)

    conn.commit()

#1 energia, 0 agua, * todos
def searchDevices(typeD):
    typeD = [0, 1] if typeD == '*' else [typeD]
    
    tupleList = []
    for cmdRun in typeD:
        cmd = f"""SELECT name, id
                    FROM devices
                    WHERE type = {cmdRun}"""
        
        cur = conn.cursor()
        cur.execute(cmd)

        tupleList += [[name[0],name[1]] for name in cur.fetchall()]

    devDictionary = {}
    for device in tupleList:
        devDictionary[device[0]] = device[1]
    
    return devDictionary

def registerDevice(addr,typeD,data):
    cmd = """INSERT INTO devices(name,type) VALUES (?,?)"""
    c.execute(cmd,(addr,typeD))

    devList = searchDevices(typeD)
    devId = devList[addr]

    if typeD: #energia
        cmd = f""" CREATE TABLE IF NOT EXISTS {"e"+str(devId)} (  date timestamp PRIMARY KEY,
                                                                P1 REAL,P2 REAL,P3 REAL,
                                                                S1 REAL,S2 REAL,S3 REAL,
                                                                FP1 REAL,FP2 REAL,FP3 REAL,
                                                                V1 REAL,V2 REAL,V3 REAL,
                                                                I1 REAL,I2 REAL,I3 REAL,
                                                                E1 REAL,E2 REAL,E3 REAL); """
        c.execute(cmd)
        cmd = """INSERT INTO avgEnergy (id,P1,P2,P3,S1,S2,S3,FP1,FP2,FP3,V1,V2,V3,I1,I2,I3,E1,E2,E3,N) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"""
        c.execute(cmd,[devId] + 15*[0] + data + [0])
    else:
        cmd = f"""CREATE TABLE IF NOT EXISTS {"w"+str(devId)} (date timestamp PRIMARY KEY, CON REAL);"""
        c.execute(cmd)
        cmd = """INSERT INTO lastWater (id,lastMeasure) VALUES (?,?)"""
        c.execute(cmd,[devId] + data)

    conn.commit()

def writeMinute(id, addr, type, dataJSON):
    if type:
        #caso a energia total seja menor que energia registrada do inicio do ultimo registro, o dispositivo
        #possivelmente foi reiniciado. Valor sera sobrescrito
        cmd = f"SELECT E1,E2,E3 FROM avgEnergy WHERE id={id}"
        c.execute(cmd)
        lastEnergy = c.fetchall()[0]

        cmd = f"SELECT P1,P2,P3,S1,S2,S3,FP1,FP2,FP3,V1,V2,V3,I1,I2,I3,E1,E2,E3,N FROM avgEnergy WHERE id={id}"
        c.execute(cmd)
        lastData = [0 if val==None else val for val in c.fetchall()[0]]
        
        if lastEnergy[0] > dataJSON['Energia1'] and lastEnergy[1] > dataJSON['Energia2'] and lastEnergy[2] > dataJSON['Energia3']:
            with open(logPath, "a") as fp:
                fp.write(f"[{datetime.now()}] WARNING: Device with address {addr} resetted or overflowed. Some energy data won't be computed.\n")
            
            cmd = f"""REPLACE INTO avgEnergy (id,P1,P2,P3,S1,S2,S3,FP1,FP2,FP3,V1,V2,V3,I1,I2,I3,E1,E2,E3,N) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"""
            data = (id,lastData[0],lastData[1],lastData[2],lastData[3],lastData[4],lastData[5],lastData[6],
            lastData[7],lastData[8],lastData[9],lastData[10],lastData[11],lastData[12],lastData[13],lastData[14],
            dataJSON['Energia1'],dataJSON['Energia2'],dataJSON['Energia3'],lastData[-1])
            c.execute(cmd,data)

        cmd = """REPLACE INTO avgEnergy (id,P1,P2,P3,S1,S2,S3,FP1,FP2,FP3,V1,V2,V3,I1,I2,I3,E1,E2,E3,N) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"""
        
        data = ( id,lastData[0]+dataJSON['rP1'],lastData[1]+dataJSON['rP2'],lastData[2]+dataJSON['rP3'],
                    lastData[3]+dataJSON['aP1'],lastData[4]+dataJSON['aP2'],lastData[5]+dataJSON['aP3'],
                    lastData[6]+dataJSON['pF1'],lastData[7]+dataJSON['pF2'],lastData[8]+dataJSON['pF3'],
                    lastData[9]+dataJSON['Vrms1'],lastData[10]+dataJSON['Vrms2'],lastData[11]+dataJSON['Vrms3'],
                    lastData[12]+dataJSON['Irms1'],lastData[13]+dataJSON['Irms2'],lastData[14]+dataJSON['Irms3'],
                    lastData[15],lastData[16],lastData[17],
                    lastData[18]+1)
        c.execute(cmd,data)
    
    else: #agua
        #caso o consumo total seja menor que consumo registrado do inicio do ultimo registro, o dispositivo
        #possivelmente foi reiniciado. Valor sera sobrescrito
        cmd = f"SELECT lastMeasure FROM lastWater WHERE id={id}"
        c.execute(cmd)
        lastWater = c.fetchall()[0]
        if lastWater[0] > dataJSON[f'total']:
            with open(logPath, "a") as fp:
                fp.write(f"[{datetime.now()}] WARNING: Device with address {addr} resetted or overflowed. Some water data won't be computed.\n")
                
                cmd = f"""REPLACE INTO lastWater (id,lastMeasure) VALUES (?,?)"""
                c.execute(cmd,(id,dataJSON['total']))    
    conn.commit()

def writeDatabase(id, addr, type, dataJSON):
    if type: #energia
        cmd = f"SELECT P1,P2,P3,S1,S2,S3,FP1,FP2,FP3,V1,V2,V3,I1,I2,I3,E1,E2,E3,N FROM avgEnergy WHERE id={id}"
        c.execute(cmd)
        measures = list(c.fetchall()[0])
        nMeasures = measures[-1]
        measures.pop()

        #energia
        energia = [measures[-1]]
        measures.pop()
        energia += [measures[-1]]
        measures.pop()
        energia += [measures[-1]]
        measures.pop()
        energia.reverse()

        if not(nMeasures):
            with open(logPath, "a") as fp:
                fp.write(f"[{datetime.now()}] ERROR: No data acquired from {addr} for the last {timeWrite} minutes.\n")
            nMeasures = 1 #evitar divisao por 0
            eList = [0,0,0]
            eAvg = energia
        else:
            eList = [dataJSON['Energia1']-energia[0], dataJSON['Energia2']-energia[1], dataJSON['Energia3']-energia[2]]
            eAvg = [dataJSON['Energia1'], dataJSON['Energia2'], dataJSON['Energia3']]
    
        cmd = f"""INSERT INTO e{id} (date,P1,P2,P3,S1,S2,S3,FP1,FP2,FP3,V1,V2,V3,I1,I2,I3,E1,E2,E3) VALUES ({18*"?,"}?)"""
        data = [datetime.now()] + [measure/nMeasures for measure in measures] + eList
        c.execute(cmd,data)

        cmd = f"""REPLACE INTO avgEnergy (id,P1,P2,P3,S1,S2,S3,FP1,FP2,FP3,V1,V2,V3,I1,I2,I3,E1,E2,E3,N) VALUES ({19*"?,"}?)"""
        data = [id] + 15*[0] + eAvg + [0]
        c.execute(cmd,data)

    else: #agua
        cmd = f"""SELECT lastMeasure FROM lastWater WHERE id = {id}"""
        c.execute(cmd)
        lastMeasure = c.fetchall()[0][0]

        if dataJSON == {}:
            data = (datetime.now(),0)
            dataLast = (id,lastMeasure)
        else:
            data = (datetime.now(),dataJSON['total']-lastMeasure)
            dataLast = (id,dataJSON['total'])

        cmd = f""" INSERT INTO w{id} (date,CON) VALUES (?,?)"""
        c.execute(cmd,data)

        cmd = f"""REPLACE INTO lastWater (id,lastMeasure) VALUES (?,?)"""
        c.execute(cmd,dataLast)
    
    conn.commit()

def deleteDevice(id, type):
    cmd = f"""DELETE FROM devices WHERE id = {id}"""
    c.execute(cmd)

    if type:
        cmd = f"""DELETE FROM avgEnergy WHERE id = {id}"""
    else:
        cmd = f"""DELETE FROM lastWater WHERE id = {id}"""
    c.execute(cmd)
    conn.commit()

def getDateRange(tableName):
    cmd = f"""
            SELECT MIN(date) AS First,
            MAX(date) AS Last
            FROM {tableName};"""
    c.execute(cmd)

    return c.fetchall()[0]

def exeCmd(cmd):
    c.execute(cmd)
    return c.fetchall()
