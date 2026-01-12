import os

from util.auth_requests import AuthRequests
    
class HomeAssistantAPI(AuthRequests):
    def __init__(self):
        super().__init__(base_url=os.getenv("HA_URL"), token=os.getenv("HA_TOKEN"))