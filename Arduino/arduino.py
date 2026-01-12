import serial
import time

class ArduinoController:
    def __init__(self, port="COM5", baudrate=115200, timeout=0.1):
        self.arduino = serial.Serial(port, baudrate=baudrate, timeout=timeout)
        self.in_configuration = False
        
        self.MAX_PLAYERS = 2
        
        self.current_player = 0
        self.commands = {
            "START": "start",
            "DAY": "day",
            "NIGHT": "night",
            "OFF": "off",
            "START_CONFIG": "sconfig",
            "NEXT_DEVICE": "ndevice",
            "SET_DEVICE": "sdevice",
            "END_CONFIG": "econfig",
            "START_KILL": "skill",
            "END_KILL": "ekill",
            "START_REVIVE": "srevive",
            "END_REVIVE": "erevive",
            "DEAD": "dead",
            "DEAD_VOTE_USED": "dvote",
            "ALIVE": "alive"
        }
        
        self.kill_menu_options = {
            "1": "Next Player",
            "2": "Previous Player",
            "3": "Kill Current",
            "4": "Cancel",   
        }
        
        self.revive_menu_options = {
            "1": "Next Player",
            "2": "Previous Player",
            "3": "Revive Current",
            "4": "Cancel",
        }
    
        
        self.configuration_options = {
            "1": "Set Device",
            "2": "Next Device",
            "3": "End Configuration"
        }
        
        # There may already be something in the buffer; clear it
        self._await_response()
        
    def _await_response(self):
        """
        Waits for a response from the Arduino.
        """
        time.sleep(0.5)  # Small delay to allow Arduino to process
        while self.arduino.in_waiting > 0:
            response = self.arduino.readline().decode('utf-8').rstrip()
            print(f"[ARDUINO] {response}")
        
    def send_command(self, command):
        """
        Sends a command to the Arduino and returns the response.
        """
        print("Sending command to Arduino:", command)
        self.arduino.write((command + '\n').encode('utf-8'))
        
    def start(self):
        self.send_command(self.commands["START"])
        
    def start_night(self):
        self.send_command(self.commands["NIGHT"])
    
    def start_day(self):
        self.send_command(self.commands["DAY"])
        
    def kill_player_screen(self):
        self.send_command(self.commands["START_KILL"])
        self._await_response()
        self.in_kill_screen = True
        
        while self.in_kill_screen:
            self._await_response()
            for key, desc in self.kill_menu_options.items():
                print(f"{key}: {desc}")
                
            user_input = input("Enter option number: ").strip()
            if user_input == "1":
                self.send_command("nplayer")
                self.current_player = (self.current_player + 1) % self.MAX_PLAYERS
                
            elif user_input == "2":
                self.send_command("pplayer")
                self.current_player = (self.current_player - 1) % self.MAX_PLAYERS
                
            elif user_input == "3":
                self.send_command("dead")
            elif user_input == "4":
                self.send_command(self.commands["END_KILL"])
                self.in_kill_screen = False
            else:
                print("Invalid option. Please try again.")
                
    def revive_player_screen(self):
        self.send_command(self.commands["START_REVIVE"])
        self._await_response()
        self.in_revive_screen = True
        
        while self.in_revive_screen:
            self._await_response()
            for key, desc in self.revive_menu_options.items():
                print(f"{key}: {desc}")
                
            user_input = input("Enter option number: ").strip()
            if user_input == "1":
                self.send_command("nplayer")
                self.current_player = (self.current_player + 1) % self.MAX_PLAYERS
                
            elif user_input == "2":
                self.send_command("pplayer")
                self.current_player = (self.current_player - 1) % self.MAX_PLAYERS
                
            elif user_input == "3":
                self.send_command("revive")
                
            elif user_input == "4":
                self.send_command(self.commands["END_REVIVE"])
                self.in_revive_screen = False
            else:
                print("Invalid option. Please try again.")
            
    
    def start_config(self):
        self.send_command(self.commands["START_CONFIG"])
        self._await_response()
        self.in_configuration = True
        
        while self.in_configuration:
            self._await_response()
            print("Configuration Options:")
            for key, desc in self.configuration_options.items():
                print(f"{key}: {desc}")
            user_input = input("Enter option number: ").strip()
            if user_input == "1":
                self.set_device()
            elif user_input == "2":
                self.next_device()
            elif user_input == "3":
                self.end_config()
                self.in_configuration = False
            else:
                print("Invalid option. Please try again.")
        
    def next_device(self):
        self.send_command(self.commands["NEXT_DEVICE"])
        self._await_response()
    
    def set_device(self):
        self.send_command(self.commands["SET_DEVICE"])
        self._await_response()
        
    def end_config(self):
        self.send_command(self.commands["END_CONFIG"])
        self._await_response()
        
    def set_dead(self, id: int):
        self.send_command(f"{self.commands['DEAD']},{id}")
    
    def set_dead_vote_used(self, id: int):
        self.send_command(f"{self.commands['DEAD_VOTE_USED']},{id}")
        
    def set_alive(self, id: int):
        self.send_command(f"{self.commands['ALIVE']},{id}")