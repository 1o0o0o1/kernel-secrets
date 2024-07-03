import os
import subprocess 

def welcome():

    print("      _   __                     _            \n"
          "     | | / /                    | |           \n"
          "     | |/ /  ___ _ __ _ __   ___| |           \n"
          "     |    \ / _ \ '__| '_ \ / _ \ |           \n"
          "     | |\  \  __/ |  | | | |  __/ |           SELECT OPTION:\n"
          "     \_| \_/\___|_|  |_| |_|\___|_|           1. Set index;\n"
          "                                              2. Wirte secret;\n"
          "                                              3. Read secret;\n"
          "    _____                    _                4. Delete secret;\n"
          "     ___|                  | |                5. Run tests(comming soon...);\n"
          "   \ `--.  ___  ___ _ __ ___| |_ ___          \n"
          "    `--. \/ _ \/ __| '__/ _ \ __/ __|         \n"
          "   /\__/ /  __/ (__| | |  __/ |_\__ \         0 - for exit.\n"
          "   \____/ \___|\___|_|  \___|\__|___/         \n"
          "                                              \n"        
          "               .--.                           \n"
          "              /.-. '----------.               \n"
          "              \'-' .--''--''-'-'              \n"
          "               '--'                           \n"
          "                                              \n"
    )


def test():
    pass

def set_index():
    global index
    command = f'echo "{index}" > /proc/secret/set_index'
    subprocess.run(command, shell=True)
    
def write_secret(data):
    command = f'echo "{data}" > /proc/secret/set_index'
    subprocess.run(command, shell=True)

def read_secret():
    command = f'cat > /proc/secret/read'
    subprocess.run(command, shell=True)

def delete_secret():
    command = f'cat > /proc/secret/delete'
    subprocess.run(command, shell=True)

def prompt():
    global index
    choose = ""
    while(choose != "c"):
        choose = int(input("> "))
        if(choose == 0):
            return
        elif(choose == 1):
            index = input("Enter index > ")
            set_index()
        elif(choose == 2):
            data = input("Enter data > ")
            write_secret(data)
        elif(choose == 3):
            read_secret()
        elif (choose == 4):
            delete_secret()
        elif (choose == test):
            test()
        else:
            print("Incorrect input")

index = 0

def main():
    welcome()
    prompt()

if __name__ == "__main__":
    main()
