"""Modify HTTP query parameters."""
from mitmproxy import http

Servername = "http://192.168.1.7/test_get.php"

def request(flow: http.HTTPFlow) -> None:
    if Servername in flow.request.pretty_url:
        flow.request.query["Temperature"] = "10000"
        flow.request.query["Humidity"] = "10000"
