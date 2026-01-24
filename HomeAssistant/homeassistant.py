import json

from pprint import pp as pprint
from HomeAssistant.homeassistant_api import HomeAssistantAPI
from consts import EDITION_COLOURS, HA_SCRIPT_NAMES

class HomeAssistantController():
    def __init__(self):
        self.api = HomeAssistantAPI()
        self.mood_light_data = {**EDITION_COLOURS["TROUBLE_BREWING"], "brightness": 100}
        
    def _trigger_script(self, script_entity_id, data=None):
        url = f"/api/services/script/turn_on"
        
        payload = {
            "entity_id": script_entity_id,
            "variables": {}
        }
        
        if data is not None:
            payload["variables"] = data
            
        print(payload)
        
        response = self.api.post(url, data=payload)
        pprint(response)
        return response

    def trigger_gong(self):
        song_name = "Chuch Bells Version 2 by Digiffects Sound Effects Library"
        # song_name = "Single Church Bell Six Rings"
        # song_name = "Single Church Bell Random Ringing"
        
        data = {
            "song_name": song_name
        }
        self._trigger_script(HA_SCRIPT_NAMES["CHURCH_BELLS"], data=data)
        # Add "in Clocktower group" to the Home Assistant script
        
    def stop_all_alexa(self):
        self._trigger_script(HA_SCRIPT_NAMES["STOP_ALL_ALEXA"])
        
    def turn_on_lights(self):
        data = {
            "brightness": 40
        }
        self._trigger_script(HA_SCRIPT_NAMES["TURN_ON"], data=data)
        self.turn_on_mood_light()
        
    def turn_off_lights(self):
        self._trigger_script(HA_SCRIPT_NAMES["TURN_OFF"])
        self.turn_off_mood_light()
        
    def turn_on_mood_light(self):
        self._trigger_script(HA_SCRIPT_NAMES["SET_MOOD_LIGHTING"], data=self.mood_light_data)
    
    def turn_off_mood_light(self):
        self._trigger_script(HA_SCRIPT_NAMES["SET_MOOD_LIGHTING_OFF"])
        
    def set_mood_light_data(self, data):
        """
        Sets the RGB value for mood lighting from now on. Automatically sets "brightness" to 100 if not provided.
        """
        
        if "brightness" not in data:
            data["brightness"] = 100
            
        self.mood_light_data = data
        
    def set_mood_light_data_individual(self, r, g, b, brightness):
        self.mood_light_data = {
            "r": r,
            "g": g,
            "b": b,
            "brightness": brightness
        }
        
    def set_good_wins(self):
        self.set_mood_light_data({
            "r": 0,
            "g": 255,
            "b": 0
        })
        self.turn_on_mood_light()
        
    def set_evil_wins(self):
        self.set_mood_light_data({
            "r": 255,
            "g": 0,
            "b": 0
        })
        self.turn_on_mood_light()