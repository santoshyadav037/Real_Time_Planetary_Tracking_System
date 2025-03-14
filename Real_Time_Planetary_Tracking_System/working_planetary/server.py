from flask import Flask, render_template
from flask_socketio import SocketIO
from skyfield.api import load, Topos
import time
import threading
from flask_cors import CORS
import socketio
from socketio import Client

app = Flask(__name__)
CORS(app)  # Enable CORS for all routes
socketio = SocketIO(app, cors_allowed_origins="*", async_mode='threading')
# sio= Client()
# esp32_ip = 'http://192.168.1.94/'
# # Error handling Code
# esp32_connected = False
# try:
#     sio.connect(esp32_ip)
#     esp32_connected = True
#     print("Connected to ESP32")
# except Exception as e:
#     print(f"Could not connect to ESP32: {e}")
#     print("Continuing without ESP32 connection")


# Observer's Location
latitude = 26.7955571  # Example: Delhi, India
longitude = 87.2431336
elevation_m = 156.28  # Approximate elevation in meters
current_body="Sun"

@app.route('/')
def index():
    return render_template('index.html')

@socketio.on('connect')
def handle_connect():
    print('Client connected')

@socketio.on('disconnect')
def handle_disconnect():
    print('Client disconnected')

@socketio.on('observer_data')
def handle_observer_data(data):
    global longitude , latitude, elevation_m
    print("Received Observer Data:", data)
    longitude= data['longitude']
    latitude = data['latitude']
    elevation_m = data['altitude']
    socketio.emit("response", "Observer data received successfully!")


# Load planetary ephemeris data
ephemeris = load('de421.bsp')
earth = ephemeris['earth']
observer = earth + Topos(latitude_degrees=latitude, 
                        longitude_degrees=longitude, 
                        elevation_m=elevation_m)

def get_alt_az(body_name=current_body):
    if body_name in ephemeris:
        body = ephemeris[body_name]
    else:
        return None, None

    ts = load.timescale()
    current_time = ts.now()
    astrometric = (body - observer).at(current_time)
    alt, az, _ = astrometric.altaz()
    
    return round(alt.degrees, 2), round(az.degrees, 2)



@socketio.on('request_data')
def handle_request(data):
    global current_body
    """Handle specific requests for celestial body data"""
    body_name = data.get('body', 'Sun')
    current_body = body_name
    alt, az = get_alt_az(current_body)
    socketio.emit('angles_update', {'altitude': alt, 'azimuth': az, 'body': body_name})
    

def background_thread():
    """Background thread that emits celestial data every second."""
    while True:
        alt, az = get_alt_az(current_body)  # Change target body if needed
        print(f"Broadcasting: alt={alt}, az={az}")
        socketio.emit('angles_update', {'altitude': alt, 'azimuth': az, 'body': current_body})
        # sio.emit('api_update',{'alt': alt, 'az': az})
        time.sleep(1)  # Send update every second

if __name__ == '__main__':
    # Start background thread for continuous updates
    thread = threading.Thread(target=background_thread)
    thread.daemon = True
    thread.start()
    
    # Run the app
    socketio.run(app, host='0.0.0.0', port=5000, debug=True, allow_unsafe_werkzeug=True)
