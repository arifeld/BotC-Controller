import os

from pprint import pp as pprint
from SmartThings.smartthings_api import SmartThingsAPI

class SmartThingsController():
    def __init__(self):
        self.api = SmartThingsAPI()
        self.location_id = os.getenv("ROOM_LOCATION_ID")
        self.device_ids = os.getenv("ROOM_LIGHTS").split(",")
        
    def _execute_command(self, device_ids, capability, command, main="switch"):
        to_execute = []
        for device_id in device_ids:
            to_execute.append({
                "url": f"devices/{device_id}/commands",
                "data": {
                    "commands": [
                        {
                            "main": main,
                            "capability": capability,
                            "command": command
                        }
                    ]
                }
            })
        
        data = self.api.batch_post(to_execute)
        pprint(data)
        return data
        
    def turn_on_room_lights(self):
        self._execute_command(self.device_ids, "switch", "on")
        
            
    def turn_off_room_lights(self):
        self._execute_command(self.device_ids, "switch", "off")