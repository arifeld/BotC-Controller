import os
from requests import Session
import threading

class AuthRequests(Session):    
    def __init__(self, base_url, token):
        super().__init__()
        self.base_url = base_url
        self.token = token
        
    def _make_request(self, method, url, *args, **kwargs):
        joined_url = self.base_url + url
        
        headers = {
            "Authorization": "Bearer " + self.token,
            "Content-Type": "application/json"
        }
        
        return super().request(method, joined_url, headers=headers, *args, **kwargs)
    
    def get(self, url):
        r = self._make_request("GET", url)     
        return r.json()
    
    def post(self, url, data, mutable_ret: list|None = None):
        r = self._make_request("POST", url, json=data)
        
        r.raise_for_status()     
        
        if mutable_ret is not None:
            mutable_ret.append(r.json())
        
        return r.json()
    
    def batch_post(self, commands):
        threads = []
        results = []
        for command in commands:
            thread = threading.Thread(target=self.post, args=(command['url'], command['data'], results))
            thread.start()
            threads.append(thread)
        
        for thread in threads:
            thread.join()
            
        return results