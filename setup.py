#from crontab import CronTab
from dbUtils import createDatabase, dbPath, dbFolder
import os
import sys

if __name__ == "__main__":
    createDatabase()
    print('Database created.')
    
    nowFolder = os.path.abspath("readDevices.py").replace("readDevices.py","")

    # username = input('Enter your username: ')
    # my_cron = CronTab(user=username)
    # job = my_cron.new(command="* * * * * cd {nowFolder} && python3 readDevices.py")
    
    print(f'You need to setup a cronjob. On a terminal, type \"crontab -e\", press i to enter insert mode and add \"* * * * * cd {nowFolder} && {sys.executable} readDevices.py\". Then, press ESC, type :wq and press ENTER to save changes.')