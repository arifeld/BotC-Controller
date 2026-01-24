from HomeAssistant.homeassistant import HomeAssistantController
from consts import EDITION_COLOURS, GAME_PHASE
from dotenv import load_dotenv
from SmartThings.smartthings import SmartThingsController
from Arduino.arduino import ArduinoController

import pyautogui

from statemachine import StateMachine, State, Event

load_dotenv()

class BOTCController(StateMachine):
    game_configuration = State("Game Configuration", initial=True, value=GAME_PHASE.CONFIGURATION)
    pregame = State("Pre-Game", value=GAME_PHASE.PRE_GAME)
    first_night = State("First Night", value=GAME_PHASE.FIRST_NIGHT)
    day_phase = State("Day Phase", value=GAME_PHASE.DAY)
    nominations_phase = State("Nominations Phase", value=GAME_PHASE.NOMINATIONS)
    prereveal_phase = State("Pre-Day Reveal Phase", value=GAME_PHASE.PRE_REVEAL)
    night_phase = State("Night Phase", value=GAME_PHASE.NIGHT)
    postgame = State("Post-Game", value=GAME_PHASE.POST_GAME)
    
    states = {
        GAME_PHASE.CONFIGURATION: game_configuration,
        GAME_PHASE.PRE_GAME: pregame,
        GAME_PHASE.FIRST_NIGHT: first_night,
        GAME_PHASE.DAY: day_phase,
        GAME_PHASE.NOMINATIONS: nominations_phase,
        GAME_PHASE.PRE_REVEAL: prereveal_phase,
        GAME_PHASE.NIGHT: night_phase,
        GAME_PHASE.POST_GAME: postgame
    }
    
    finish_config = Event(
        states[GAME_PHASE.CONFIGURATION].to(states[GAME_PHASE.PRE_GAME]),
        name="Finish game configuration"
    )
    
    start = Event(
        states[GAME_PHASE.PRE_GAME].to(states[GAME_PHASE.FIRST_NIGHT]),
        name="Start the game"
    )
    
    first_day = Event(
        states[GAME_PHASE.FIRST_NIGHT].to(states[GAME_PHASE.DAY]),
        name="Start the first day"
    )
    
    start_nominations = Event(
        states[GAME_PHASE.DAY].to(states[GAME_PHASE.NOMINATIONS]),
        name="Start nominations"
    )
    
    skip_nominations = Event(
        states[GAME_PHASE.DAY].to(states[GAME_PHASE.NIGHT]),
        name="Skip nominations"
    )
    
    start_prereveal = Event(
        states[GAME_PHASE.NIGHT].to(states[GAME_PHASE.PRE_REVEAL]),
        name="Start pre-day reveal phase"
    )

    start_day = Event(
        states[GAME_PHASE.PRE_REVEAL].to(states[GAME_PHASE.DAY]),
        name="Start day phase"
    )
    
    start_night = Event(
        states[GAME_PHASE.NOMINATIONS].to(states[GAME_PHASE.NIGHT]),
        name="Start night phase"
    )
    

    end_game_via_nomination = Event(
        states[GAME_PHASE.NOMINATIONS].to(states[GAME_PHASE.POST_GAME]),
        name="End game via nomination"
    )
    
    end_game_via_night = Event(
        states[GAME_PHASE.NIGHT].to(states[GAME_PHASE.POST_GAME]),
        name="End game via night"
    )
    
    revert_end_game = Event(
        states[GAME_PHASE.POST_GAME].to(states[GAME_PHASE.DAY]),
        name="Revert from post-game to day phase"
    )
    
    restart_game = Event(
        states[GAME_PHASE.POST_GAME].to(states[GAME_PHASE.CONFIGURATION]),
        name="Restart the game"
    )
    
    progression_options = {
        GAME_PHASE.CONFIGURATION: [{
            "label": "Finish Configuration",
            "input": "1",
            "event": "finish_config"
        },{
            "label": "Set Trouble Brewing",
            "input": "2",
            "non_state_event": "set_trouble_brewing"
        }, {
            "label": "Set Bad Moon Rising",
            "input": "3",
            "non_state_event": "set_bad_moon_rising"
        },{
            "label": "Set Sects and Violets",
            "input": "4",
            "non_state_event": "set_sects_and_violets"
        },
        {
            "label": "Configure Arduino Devices",
            "input": "5",
            "non_state_event": "configure_arduino"
        }],
        GAME_PHASE.PRE_GAME: [{
            "label": "Start Game",
            "input": "1",
            "event": "start"
        }],
        GAME_PHASE.FIRST_NIGHT: [{
            "label": "Start First Day",
            "input": "1",
            "event": "first_day"
        }],
        GAME_PHASE.PRE_REVEAL: [{
            "label": "Start Day Phase",
            "input": "1",
            "event": "start_day"
        }],
        GAME_PHASE.DAY: [{
            "label": "Start Nominations",
            "input": "1",
            "event": "start_nominations"
        },
        {
            "label": "Set Player Dead",
            "input": "2",
            "non_state_event": "start_player_kill_screen"
        },
        {
            "label": "Set Player Alive",
            "input": "3",
            "non_state_event": "start_player_revive_screen"
        }, 
        {
            "label": "Start Night Phase (Skip Nominations)",
            "input": "4",
            "event": "skip_nominations"
        },
        {
            "label": "Stop Alexa",
            "input": "5",
            "non_state_event": "stop_all_alexa"
        }],
        GAME_PHASE.NOMINATIONS: [{
            "label": "Start Night Phase",
            "input": "1",
            "event": "start_night"
        }, {
            "label": "Restart Nomination Config",
            "input": "2",
            "non_state_event": "start_nomination_config"   
        },{
            "label": "Kill Player",
            "input": "3",
            "non_state_event": "start_player_kill_screen"
        },
        {
            "label": "End Game via Nomination",
            "input": "4",
            "event": "end_game_via_nomination"
        },
        {
            "label": "Stop Alexa",
            "input": "5",
            "non_state_event": "stop_all_alexa"
        }],
        GAME_PHASE.NIGHT: [{
            "label": "Start Pre-Day Phase",
            "input": "1",
            "event": "start_prereveal"            
        },
        {
            "label": "Set Player Dead",
            "input": "2",
            "non_state_event": "start_player_kill_screen"
        },
        {
            "label": "Set Player Alive",
            "input": "3",
            "non_state_event": "start_player_revive_screen"
        },
        {
            "label": "End Game via Night",
            "input": "4",
            "event": "end_game_via_night"
        }],
        GAME_PHASE.POST_GAME: [{
            "label": "Go Back",
            "input": "1",
            "event": "revert_end_game"
        }, {
            "label": "Good Wins",
            "input": "2",
            "non_state_event": "set_good_wins"
        }, {
            "label": "Evil Wins",
            "input": "3",
            "non_state_event": "set_evil_wins"
        }, {
            "label": "Restart Game",
            "input": "4",
            "non_state_event": "restart_game"
        }]
    }
    

    def __init__(self):
        super().__init__()
        self.smartthings_controller = SmartThingsController()
        self.homeassistant_controller = HomeAssistantController()
        
        self.arduino_controller = ArduinoController()
    
    @pregame.enter
    def entering_pre_game(self):
        print("Entering Pre-Game phase...")
        self.homeassistant_controller.turn_on_mood_light()
        self.arduino_controller.start()
            
    # Game phase actions
    @first_night.enter
    def entering_first_night(self):
        print("Entering First Night phase...")
        # self.smartthings_controller.turn_off_room_lights()
        pyautogui.press("playpause")
        self.homeassistant_controller.turn_off_lights()
        self.arduino_controller.start_night()

    @first_night.exit
    def exiting_first_night(self):
        print("Exiting First Night phase...")
        self.homeassistant_controller.turn_on_lights()
        pyautogui.press("volumedown", presses=50, interval=0.05)
        pyautogui.press("playpause")

    @prereveal_phase.enter
    def entering_prereveal_phase(self):
        print("Entering Pre-Reveal phase...")
        self.homeassistant_controller.turn_on_lights()
        pyautogui.press("volumedown", presses=50, interval=0.05)
        pyautogui.press("playpause")
        self.arduino_controller.start_prereveal()

    @day_phase.enter
    def entering_day_phase(self):
        print("Entering Day phase...")
        # self.smartthings_controller.turn_on_room_lights()
        self.arduino_controller.start_day()
        


    @nominations_phase.enter
    def entering_nominations_phase(self):
        print("Entering Nominations phase...")
        self.homeassistant_controller.trigger_gong()
        self.arduino_controller.start_nomination_config()
        
    @night_phase.enter
    def entering_night_phase(self):
        print("Entering Night phase...")
        # self.smartthings_controller.turn_off_room_lights() 
        self.homeassistant_controller.turn_off_lights()
        self.arduino_controller.start_night()
        pyautogui.press("nexttrack")
        pyautogui.press("volumeup", presses=50, interval=0.05)
    
    @postgame.enter
    def entering_post_game(self):
        print("Entering Post-Game phase...")
        # self.smartthings_controller.turn_off_room_lights() 
        self.arduino_controller.enter_end_game()
        
    def send_non_state(self, event_name):
        if event_name == "set_trouble_brewing":
            print("Setting Trouble Brewing edition...")
            self.homeassistant_controller.set_mood_light_data(EDITION_COLOURS["TROUBLE_BREWING"])
        elif event_name == "set_bad_moon_rising":
            print("Setting Bad Moon Rising edition...")
            self.homeassistant_controller.set_mood_light_data(EDITION_COLOURS["BAD_MOON_RISING"])
        elif event_name == "set_sects_and_violets":
            print("Setting Sects and Violets edition...")
            self.homeassistant_controller.set_mood_light_data(EDITION_COLOURS["SECTS_AND_VIOLETS"])
        elif event_name == "stop_all_alexa":
            print("Stopping all Alexa devices...")
            self.homeassistant_controller.stop_all_alexa()
        elif event_name == "configure_arduino":
            print("Starting Arduino device configuration...")
            self.arduino_controller.start_config()
            
        elif event_name == "start_player_kill_screen":
            print("Starting player kill screen...")
            self.arduino_controller.kill_player_screen()
            
        elif event_name == "start_player_revive_screen":
            print("Starting player revive screen...")
            self.arduino_controller.revive_player_screen()
            
        elif event_name == "set_player_dead":
            player_id = input("Enter Player ID to set as dead: ").strip()
            if player_id.isdigit():
                self.arduino_controller.set_dead(int(player_id))
            else:
                print("Invalid Player ID. Must be an integer.")
        elif event_name == "set_player_alive":
            player_id = input("Enter Player ID to set as alive: ").strip()
            if player_id.isdigit():
                self.arduino_controller.set_alive(int(player_id))
            else:
                print("Invalid Player ID. Must be an integer.")
        elif event_name == "start_nomination_config":
            print("Starting nomination configuration...")
            self.arduino_controller.start_nomination_config()
            
        elif event_name == "set_good_wins":
            print("Setting Good Wins...")
            self.homeassistant_controller.set_good_wins()
            self.arduino_controller.set_good_wins()

        elif event_name == "set_evil_wins":
            print("Setting Evil Wins...")
            self.homeassistant_controller.set_evil_wins()
            self.arduino_controller.set_evil_wins()
            
        elif event_name == "restart_game":
            print("Restarting the game...")
            self.homeassistant_controller.turn_on_mood_light()
            self.arduino_controller.restart_game()
            self.restart_game()
            
        else:
            print(f"Unknown non-state event: {event_name}")

        
    

class EventController():
    
    
    
    def __init__(self):
        self.botc = BOTCController()
        
    def start_game(self):
        while True:
            options = self.botc.progression_options.get(self.botc.current_state.value, [])
            print("\nAvailable Actions:")
            for option in options:
                print(f"{option['input']}: {option['label']}")
            
            
            next_action = input(f"Next action: ").strip().lower()
            matched_option = next((opt for opt in options if opt['input'] == next_action), None)
            if matched_option is not None:
                if "event" in matched_option:
                    self.botc.send(matched_option['event'])
                elif "non_state_event" in matched_option:
                    self.botc.send_non_state(matched_option['non_state_event'])
            else:
                print("Invalid input. Please try again.")
            
            
            

            
    def _draw_graph(self):
        self.botc._graph().write_png("botc_state_machine.png")
          
    

def main():    
    controller = EventController()
    controller._draw_graph()
    controller.start_game()

if __name__ == "__main__":
    main()