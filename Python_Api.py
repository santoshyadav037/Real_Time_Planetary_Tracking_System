from flask import Flask, jsonify
from skyfield.api import load, Topos
from datetime import datetime, timezone
import time

app = Flask(__name__)

# Observer's Location
latitude = 26.7955571  # Example: Delhi, India
longitude = 87.2431336
elevation_m = 156.28  # Approximate elevation in meters

# Load planetary ephemeris data
ephemeris = load('de421.bsp')
earth = ephemeris['earth']
observer = earth + Topos(latitude_degrees=latitude, longitude_degrees=longitude, elevation_m=elevation_m)

def get_alt_az(body_name="Mars"):
    if body_name in ephemeris:
        body = ephemeris[body_name]
    else:
        return None, None

    ts = load.timescale()
    current_time = ts.now()
    astrometric = (body - observer).at(current_time)
    alt, az, _ = astrometric.altaz()
    
    return round(alt.degrees, 2), round(az.degrees, 2)

@app.route('/angles', methods=['GET'])
def send_angles():
    alt, az = get_alt_az("Sun")  # Change target body if needed
    print(alt, az)
    return jsonify({"altitude": alt, "azimuth": az})

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)  # Allow access from ESP32
