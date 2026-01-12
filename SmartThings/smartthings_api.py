import os

from util.auth_requests import AuthRequests
    
class SmartThingsAPI(AuthRequests):
    ROOT_URL = "https://api.smartthings.com/v1/"
    
    def __init__(self):
        super().__init__(base_url=self.ROOT_URL, token=os.getenv("PAT"))