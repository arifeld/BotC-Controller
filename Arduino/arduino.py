import serial
import serial.tools.list_ports
import time

class ArduinoController:
    def __init__(self, baudrate=115200, timeout=0.1):
        
        print("COM devices:")
        print([comport.device for comport in serial.tools.list_ports.comports()])
        port = input("Enter the COM port for the Arduino (e.g., COM5, unlikely to be COM1): ").strip()
        
        self.arduino = serial.Serial(port, baudrate=baudrate, timeout=timeout)
        self.in_configuration = False
        self.in_nomination_config = False
        
        self.MAX_PLAYERS = 15
        
        self.player_count = 15
        
        self.current_player = 0
        self.commands = {
            "START": "start",
            "DAY": "day",
            "NIGHT": "night",
            "OFF": "off",
            "PRE_REVEAL": "prerev",
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
            "ALIVE": "alive",
            
            "NEXT_PLAYER": "nplayer",
            "PREVIOUS_PLAYER": "pplayer",
            "SET_PLAYER": "splayer",
            
            "START_NOMINATION_CONFIG": "snomcon",
            "START_NOMINATIONS": "snomin",
            "END_NOMINATIONS": "enomin",
            "VOTE_YES": "vyes",
            "VOTE_NO": "vno",
            "VOTE_SKIP": "vskip",
            
            "END_GAME": "endgame",
            "GOOD_WINS": "goodwins",
            "EVIL_WINS": "evilwins",
        }
        
        self.kill_menu_options = {
            "1": "Next Player",
            "2": "Previous Player",
            "3": "Kill Current",
            "4": "Cancel",   
            "5": "Set Player"
        }
        
        self.revive_menu_options = {
            "1": "Next Player",
            "2": "Previous Player",
            "3": "Revive Current",
            "4": "Cancel",
            "5": "Set Player"
        }
    
        self.configuration_options = {
            "1": "Set Device",
            "2": "Next Device",
            "3": "End Configuration"
        }
        
        self.nomination_config_options = {
            "1": "Next Player",
            "2": "Previous Player",
            "3": "Start Nominations",
            "4": "Cancel",
            "5": "Set Player"
        }
        
        self.nominations_options = {
            "1": "Voted Yes",
            "2": "Voted No",
            "3": "Skipped",
            "4": "End Nominations"
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
        
    def _get_set_player(self):
        player_id = input("Enter player ID to set as current: ").strip()
        if player_id.isdigit() and 0 <= int(player_id) < self.player_count:
            self.current_player = int(player_id)
            self.send_command(f"{self.commands["SET_PLAYER"]},{self.current_player}")
        else:
            print("Invalid player ID. Please try again.")

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
    
    def start_prereveal(self):
        self.send_command(self.commands["PRE_REVEAL"])

    def start_day(self):
        self.send_command(self.commands["DAY"])
        
    def kill_player_screen(self):
        self.send_command(self.commands["START_KILL"])
        self._await_response()
        self.in_kill_screen = True
        
        while self.in_kill_screen:
            print("Current player index:", self.current_player)

            self._await_response()
            for key, desc in self.kill_menu_options.items():
                print(f"{key}: {desc}")
                
            user_input = input("Enter option number: ").strip()
            if user_input == "1":
                self.send_command(self.commands["NEXT_PLAYER"])
                self.current_player = (self.current_player + 1) % self.player_count
                
            elif user_input == "2":
                self.send_command(self.commands["PREVIOUS_PLAYER"])
                self.current_player = (self.current_player - 1) % self.player_count

            elif user_input == "3":
                self.send_command(self.commands["DEAD"])
            elif user_input == "4":
                self.send_command(self.commands["END_KILL"])
                self.in_kill_screen = False
            elif user_input == "5":
                self._get_set_player()
            else:
                print("Invalid option. Please try again.")
                
    def revive_player_screen(self):
        self.send_command(self.commands["START_REVIVE"])
        self._await_response()
        self.in_revive_screen = True
        
        print("Current player index:", self.current_player)
        
        while self.in_revive_screen:
            self._await_response()
            for key, desc in self.revive_menu_options.items():
                print(f"{key}: {desc}")
                
            user_input = input("Enter option number: ").strip()
            if user_input == "1":
                self.send_command(self.commands["NEXT_PLAYER"])
                self.current_player = (self.current_player + 1) % self.player_count
                
            elif user_input == "2":
                self.send_command(self.commands["PREVIOUS_PLAYER"])
                self.current_player = (self.current_player - 1) % self.player_count
                
            elif user_input == "3":
                self.send_command(self.commands["START_REVIVE"])
                
            elif user_input == "4":
                self.send_command(self.commands["END_REVIVE"])
                self.in_revive_screen = False

            elif user_input == "5":
                self._get_set_player()

            else:
                print("Invalid option. Please try again.")
            
    
    def start_config(self):
        self.send_command(self.commands["START_CONFIG"])
        self._await_response()
        self.in_configuration = True
        
        self.player_count = 0
        
        while self.in_configuration:
            self._await_response()
            print("Configuration Options:")
            for key, desc in self.configuration_options.items():
                print(f"{key}: {desc}")
            user_input = input("Enter option number: ").strip()
            if user_input == "1":
                self.set_device()
                self.player_count += 1
            elif user_input == "2":
                self.next_device()
            elif user_input == "3":
                self.end_config()
                print("Configuration complete. Total players configured:", self.player_count)
                self.in_configuration = False
            else:
                print("Invalid option. Please try again.")
    

    def start_nomination_config(self):
        self.in_nomination_config = True
        self.send_command(self.commands["START_NOMINATION_CONFIG"])
        while self.in_nomination_config:
            self._await_response()
            print("Nomination Configuration Options:")
            for key, desc in self.nomination_config_options.items():
                print(f"{key}: {desc}")
            user_input = input("Enter option number: ").strip()
            if user_input == "1":
                self.send_command(self.commands["NEXT_PLAYER"])
                self.current_player = (self.current_player + 1) % self.player_count

            elif user_input == "2":
                self.send_command(self.commands["PREVIOUS_PLAYER"])
                self.current_player = (self.current_player - 1) % self.player_count

            elif user_input == "3":
                self.send_command(self.commands["START_NOMINATIONS"])
                self.in_nomination_config = False
                self.in_nominations = True
                self.start_nominations()

            elif user_input == "4":
                self.send_command(self.commands["END_NOMINATIONS"])
                self.in_nomination_config = False

            elif user_input == "5":
                self._get_set_player()
                
            else:
                print("Invalid option. Please try again.")
                
    def start_nominations(self):
        while self.in_nominations:
            print("Nominations Options:")
            for key, desc in self.nominations_options.items():
                print(f"{key}: {desc}")
            user_input = input("Enter option number: ").strip()
            if user_input == "1":
                self.send_command(self.commands["VOTE_YES"])
                self.current_player = (self.current_player + 1) % self.player_count
            elif user_input == "2":
                self.send_command(self.commands["VOTE_NO"])
                self.current_player = (self.current_player + 1) % self.player_count
            elif user_input == "3":
                self.send_command(self.commands["VOTE_SKIP"])
                self.current_player = (self.current_player + 1) % self.player_count
            elif user_input == "4":
                self.send_command(self.commands["DAY"])
                time.sleep(1)
                self.send_command(self.commands["END_NOMINATIONS"])
                self.in_nominations = False
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
        
    def enter_end_game(self):
        self.send_command(self.commands['END_GAME'])
        
    def set_good_wins(self):
        self.send_command(self.commands['GOOD_WINS'])
        
    def set_evil_wins(self):
        self.send_command(self.commands['EVIL_WINS'])
        
    def restart_game(self):
        self.send_command(self.commands['START'])