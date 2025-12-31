from consts import GAME_PHASE
from dotenv import load_dotenv
from SmartThings.smartthings import SmartThingsController

from statemachine import StateMachine, State, Event

load_dotenv()

class BOTCController(StateMachine):
    pregame = State("Pre-Game", initial=True, value=GAME_PHASE.PRE_GAME)
    first_night = State("First Night", value=GAME_PHASE.FIRST_NIGHT)
    day_phase = State("Day Phase", value=GAME_PHASE.DAY)
    nominations_phase = State("Nominations Phase", value=GAME_PHASE.NOMINATIONS)
    night_phase = State("Night Phase", value=GAME_PHASE.NIGHT)
    postgame = State("Post-Game", final=True, value=GAME_PHASE.POST_GAME)
    
    states = {
        GAME_PHASE.PRE_GAME: pregame,
        GAME_PHASE.FIRST_NIGHT: first_night,
        GAME_PHASE.DAY: day_phase,
        GAME_PHASE.NOMINATIONS: nominations_phase,
        GAME_PHASE.NIGHT: night_phase,
        GAME_PHASE.POST_GAME: postgame
    }
    
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
    
    start_night = Event(
        states[GAME_PHASE.NOMINATIONS].to(states[GAME_PHASE.NIGHT]),
        name="Start night phase"
    )
    
    start_day = Event(
        states[GAME_PHASE.NIGHT].to(states[GAME_PHASE.DAY]),
        name="Start day phase"
    )
    
    end_game_via_nomination = Event(
        states[GAME_PHASE.NOMINATIONS].to(states[GAME_PHASE.POST_GAME]),
        name="End game via nomination"
    )
    
    end_game_via_night = Event(
        states[GAME_PHASE.NIGHT].to(states[GAME_PHASE.POST_GAME]),
        name="End game via night"
    )
    
    progression_options = {
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
        GAME_PHASE.DAY: [{
            "label": "Start Nominations",
            "input": "1",
            "event": "start_nominations"
        },
        {
            "label": "Start Night Phase (Skip Nominations)",
            "input": "2",
            "event": "skip_nominations"
        }],
        GAME_PHASE.NOMINATIONS: [{
            "label": "Start Night Phase",
            "input": "1",
            "event": "start_night"
        },
        {
            "label": "End Game via Nomination",
            "input": "2",
            "event": "end_game_via_nomination"
        }],
        GAME_PHASE.NIGHT: [{
            "label": "Start Day Phase",
            "input": "1",
            "event": "start_day"            
        },
        {
            "label": "End Game via Night",
            "input": "2",
            "event": "end_game_via_night"
        }]
    }
    

    def __init__(self):
        super().__init__()
        self.smartthings_controller = SmartThingsController()
        
        
    # Game phase actions
    @first_night.enter
    def entering_first_night(self):
        print("Entering First Night phase...")
        self.smartthings_controller.turn_off_lights()

    @day_phase.enter
    def entering_day_phase(self):
        print("Entering Day phase...")
        self.smartthings_controller.turn_on_lights()
        
    @night_phase.enter
    def entering_night_phase(self):
        print("Entering Night phase...")
        self.smartthings_controller.turn_off_lights()
        
    
        
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
                self.botc.send(matched_option['event'])
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