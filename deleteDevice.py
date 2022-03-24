from dbUtils import searchDevices, deleteDevice

def inputDelete():
    try:
        x = int(input('What\'s the type of the device to be deleted: (0)Water, (1)Energy: '))
        type = x

        devices = searchDevices(x)
        if devices == {}:
            print('No devices found.')
            return False

        index = 0
        for device in devices.keys():
            print(f"{index}. {device}")
            index += 1

        x = int(input('>> '))
        addr = list(devices.keys())[x]
        id = devices[addr]

        deleteDevice(id,type)
        print('Device deleted.')
    except:
        print('Invalid input')

if __name__ == "__main__":
    inputDelete()