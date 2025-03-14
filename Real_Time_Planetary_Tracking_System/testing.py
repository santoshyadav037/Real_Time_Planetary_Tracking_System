import asyncio
import websockets
from flask import Flask,jsonify
from skyfield.api import Topos, load
from datetime import datetime,timezone
import time

app = Flask(__name__)


# Updated handler function - no path parameter in newer websockets versions
async def handle_client(websocket):
    try:
        async for message in websocket:
            print(f"Received: {message}")
            await websocket.send(f"Server received: {message}")
    except Exception as e:
        print(f"Error handling client: {e}")

async def main():
    server = await websockets.serve(handle_client, "0.0.0.0", 8765)
    print(f"WebSocket server started on port 8765")
    await server.wait_closed()

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("Server stopped by user")
