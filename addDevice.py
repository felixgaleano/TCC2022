import requests as req

from dbUtils import registerDevice, searchDevices

if __name__ == "__main__":
    addr = input('Insert device address: ')

    if addr in searchDevices('*'):
        print('Device already registered')
        quit()

    try:
        res = req.get('http://' + addr + '/data').json()

        #type = false => med. de agua 
        #type =  true => med. de energia
        type = False if 'total' in res.keys() else True        

        data = [res['Energia1'],res['Energia2'],res['Energia3']] if type else [res['total']]
        registerDevice(addr,type,data)
        print('Device added.')
        
    except:
        print('Couldn\'t get a response from the specified address.')