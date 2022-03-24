try:
    import json
    import matplotlib.pyplot as plt
    import matplotlib.dates as mdates
    from dbUtils import exeCmd, getDateRange, searchDevices
    from datetime import datetime, timedelta
except:
    print("Missing libs. Try running \"python3 -m pip install -r requirements.txt\"")
    quit()

with open('settings.json', "r") as file:
    settings = json.load(file)
nHour = int(60/settings['writeOnDbInterval'])

def inputProprieties():
    try:
        x = int(input('What\'s the type of the device to display: (0)Water, (1)Energy: '))
        typeD = int(x)


        devices = searchDevices(x)
        if devices == {}:
            print('No devices found.')
            quit()
        index = 0
        print('Select device:')
        for device in devices.keys():
            print(f"{index}. {device}")
            index += 1
        x = int(input('>> '))
        addr = list(devices.keys())[x]
        id = str(devices[addr])

        if typeD:
            tableName = 'e' + id
            
            print('Enter which propriety you want to display.')
            x = input("Proprieties available: P, Ptotal, S, Stotal, E, Etotal, FP, V, I: ")
            values = x.replace(' ','').split(',')

            props = {}
            for prop in values: 
                variable = prop[0].upper() if prop[0].upper()!='F' else 'FP'
                if 'total' in prop:
                    props[prop] = [f'{variable}1+{variable}2+{variable}3']
                else:
                    props[prop] = [f"{variable}1",f"{variable}2",f"{variable}3"]

        else:
            props = {'consumo': ['CON']}
            tableName = 'w' + id

        dateRange = getDateRange(tableName)
        print(f'Data available from {dateRange[0]} to {dateRange[1]}')
        startDate = input('Insert start date (yyyy-MM-dd): ').replace('-','/')
        startDate = datetime.strptime(startDate, f"%Y/%m/%d")
        x = input('Range (0) Day, (1) Week, (2) Month, (3) Year, (4) start-now: ')
        range = ['day','week','month','year','all'][int(x)]
        
        if range == 'day'  : endDate = startDate + timedelta(days=1)
        if range == 'week' : endDate = startDate + timedelta(days=7)
        if range == 'month': endDate = startDate + timedelta(days=30)
        if range == 'year' : endDate = startDate + timedelta(days=365)
        if range == 'all'  : endDate = datetime.strptime(dateRange[1][:10], f"%Y-%m-%d") + timedelta(days=1)

        dates = [startDate.strftime(f'%Y-%m-%d'), endDate.strftime(f'%Y-%m-%d')]
        return tableName, props, dates

    except Exception as e:
        print(e)
        print('Invalid input')
        quit()


def getYLabel(tag):
    #P, Ptotal, S, Stotal, E, Etotal, FP, V, I, consumo
    x=tag.upper()
    if x in ['P','PTOTAL']: lab = 'P (kW)'
    if x in ['S','STOTAL']: lab = 'S (VA)'
    if x in ['E','ETOTAL']: lab = 'E (kWh) hour'
    if x=='FP': lab = 'Power Factor'
    if x=='V': lab = 'Vrms (V)'
    if x=='I': lab = 'Irms (A)'
    if x=='CONSUMO': lab = 'Consumo (L) hour'

    return lab

if __name__ == "__main__":
    tableName, props, dates = inputProprieties()

    strVar = ''
    for prop in props.keys():   
        for col in props[prop]:
            strVar += col + ','
    strVar = strVar[:-1]
    strVar = strVar.upper()
    
    cmd = F"""  SELECT date,{strVar}
                FROM {tableName} 
                WHERE JULIANDAY(date)>=JULIANDAY('{dates[0]}') 
                        AND JULIANDAY(date)<JULIANDAY('{dates[1]}')"""
    resQuery = exeCmd(cmd)
    if resQuery == []:
        print('No data on that range.')
        quit()

    index = 0
    data = []
    for i in range(len(resQuery[0])):
        temp = []
        for inf in resQuery:
            temp.append(inf[index])
        index+=1
        data.append(temp)
    data[0] = [datetime.strptime(d,f'%Y-%m-%d %H:%M:%S.%f') for d in data[0]]

    dataDic = {}
    dataDic['date'] = data[0]
    index = 1
    for prop in props.keys():
        dataDic[prop] = {}
        for val in props[prop]:
            dataDic[prop][val] = data[index]
            index +=1

    fig, axs = plt.subplots(len(props.keys()),1)

    index = 1
    axIndex = 0
    #P, Ptotal, S, Stotal, E, Etotal, FP, V, I, consumo
    for d in dataDic.keys():
        if d == 'date': continue

        if len(props.keys())!=1:   ax = axs[axIndex]
        else:   ax = axs

        cte = 1 if d.upper() in ['FP','V','I','CONSUMO','E','ETOTAL'] else 1/1e3
        for col in dataDic[d].keys():
            yContent = [val*cte for val in dataDic[d][col]]

            leg = col if col!='CON' else 'Consumo'
            if d.upper() in ['E','ETOTAL','CONSUMO']:
                yHour = []
                xHour = []
                for i in range(nHour,len(dataDic['date']),nHour):
                    xHour.append(dataDic['date'][i])
                    yHour.append(sum(dataDic[d][col][i-nHour:i]))
                ax.plot(xHour,yHour,label=leg)
            else:
                ax.plot(dataDic['date'],yContent,label=leg)
            
            index +=1
        #ax.set_xlabel('time')
        ax.set_ylabel(getYLabel(d))
        ax.grid(True)
        ax.legend()
        axIndex += 1

    fig.tight_layout()
    plt.gca().xaxis.set_major_formatter(mdates.DateFormatter(f'%Y-%m-%d %H:%M'))
    plt.gcf().autofmt_xdate()
    plt.show()