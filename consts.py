from enum import Enum

class GAME_PHASE(Enum):
    PRE_GAME = "pregame"
    FIRST_NIGHT = "first_night"
    DAY = "day_phase"
    NOMINATIONS = "nomination_phase"
    NIGHT = "night_phase"
    POST_GAME = "postgame"