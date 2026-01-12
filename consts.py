from enum import Enum

class GAME_PHASE(Enum):
    CONFIGURATION = "configuration"
    PRE_GAME = "pregame"
    FIRST_NIGHT = "first_night"
    DAY = "day_phase"
    NOMINATIONS = "nomination_phase"
    NIGHT = "night_phase"
    POST_GAME = "postgame"
    
EDITION_COLOURS = {
    "TROUBLE_BREWING": {
        "r": 159,
        "g": 9,
        "b": 36
    },
    "BAD_MOON_RISING": {
        "r": 206,
        "g": 111,
        "b": 37
    },
    "SECTS_AND_VIOLETS": {
        "r": 76,
        "g": 54,
        "b": 130
    }
}
    
HA_SCRIPT_NAMES = {
   "TURN_OFF": "script.turn_off_all_lights",
   "TURN_ON": "script.turn_on_all_lights",
   "CHURCH_BELLS": "script.trigger_bell_sounds",
   "STOP_ALL_ALEXA": "script.stop_all_alexa",
   "SET_MOOD_LIGHTING": "script.set_mood_lighting",
   "SET_MOOD_LIGHTING_OFF": "script.set_mood_lighting_off"
}