from flask import Flask, request, jsonify
from flask_cors import CORS
import ephem
import math
import datetime
import time
from functools import wraps

# Set this to True when you want to enable authentication
ENABLE_AUTH = False  # <-- Change to True when ready for production

app = Flask(__name__)
# Better CORS configuration
CORS(app, resources={r"/*": {"origins": "*"}}, 
     allow_headers=["Content-Type", "Authorization"])

# API versioning
API_VERSION = "/api/v1"

# Simple authentication credentials (use environment variables in production)
USERNAME = "telescope"
PASSWORD = "secure_password"

# Observer location (default values)
observer_location = {
    "latitude": 40.7128,  # New York latitude
    "longitude": -74.0060,  # New York longitude
    "elevation": 10.0  # meters
}

def requires_auth(f):
    @wraps(f)
    def decorated(*args, **kwargs):
        # Skip authentication check if disabled
        if not ENABLE_AUTH:
            return f(*args, **kwargs)
            
        auth = request.authorization
        if not auth or auth.username != USERNAME or auth.password != PASSWORD:
            return error_response("Authentication required", 401)
        return f(*args, **kwargs)
    return decorated

def error_response(message, status_code=400):
    """Create standardized error responses"""
    return jsonify({"success": False, "error": message}), status_code

def get_body_position(body_name):
    """Calculate the azimuth and altitude for a celestial body."""
    try:
        observer = ephem.Observer()
        observer.lat = str(observer_location["latitude"])
        observer.lon = str(observer_location["longitude"])
        observer.elevation = observer_location["elevation"]
        observer.date = ephem.now()
        
        # Initialize the appropriate celestial body
        if body_name == "sun":
            body = ephem.Sun()
        elif body_name == "moon":
            body = ephem.Moon()
        elif body_name == "mercury":
            body = ephem.Mercury()
        elif body_name == "venus":
            body = ephem.Venus()
        elif body_name == "mars":
            body = ephem.Mars()
        elif body_name == "jupiter":
            body = ephem.Jupiter()
        elif body_name == "saturn":
            body = ephem.Saturn()
        elif body_name == "uranus":
            body = ephem.Uranus()
        elif body_name == "neptune":
            body = ephem.Neptune()
        else:
            return {"error": f"Unknown celestial body: {body_name}"}
        
        # Compute the position
        body.compute(observer)
        
        # Convert altitude and azimuth from radians to degrees
        alt = math.degrees(body.alt)
        az = math.degrees(body.az)
        
        return {
            "altitude": alt,
            "azimuth": az
        }
    except Exception as e:
        return {"error": f"Error calculating position: {str(e)}"}

# Add header to all responses
@app.after_request
def add_header(response):
    response.headers['Content-Type'] = 'application/json'
    return response

# Health check endpoint
@app.route('/health', methods=['GET'])
def health_check():
    """Endpoint for Arduino to check if server is running"""
    return jsonify({
        "success": True,
        "server": "running",
        "time": datetime.datetime.now().isoformat()
    })

# --- API Routes ---

# Versioned routes
@app.route(f'{API_VERSION}/angles', methods=['GET'])
def get_angles_v1():
    body = request.args.get('body', 'sun')
    result = get_body_position(body)
    if "error" in result:
        return error_response(result["error"])
    return jsonify(result)

@app.route(f'{API_VERSION}/update-location', methods=['POST'])
@requires_auth  # Authentication is controlled by ENABLE_AUTH flag
def update_location_v1():
    try:
        data = request.json
        
        # Validate input
        if not all(key in data for key in ['latitude', 'longitude', 'elevation']):
            return error_response("Missing required fields")
        
        # Check if values are valid numbers
        try:
            latitude = float(data['latitude'])
            longitude = float(data['longitude'])
            elevation = float(data['elevation'])
            
            # Check if latitude and longitude are in valid ranges
            if latitude < -90 or latitude > 90:
                return error_response("Latitude must be between -90 and 90")
            if longitude < -180 or longitude > 180:
                return error_response("Longitude must be between -180 and 180")
            
            # Update observer location
            observer_location["latitude"] = latitude
            observer_location["longitude"] = longitude
            observer_location["elevation"] = elevation
            
            return jsonify({
                "success": True, 
                "message": "Location updated successfully",
                "location": observer_location
            })
            
        except ValueError:
            return error_response("Invalid numerical values")
            
    except Exception as e:
        return error_response(str(e))

@app.route(f'{API_VERSION}/get-location', methods=['GET'])
def get_location_v1():
    return jsonify(observer_location)

# Legacy non-versioned routes for backward compatibility
@app.route('/angles', methods=['GET'])
def get_angles():
    return get_angles_v1()

@app.route('/update-location', methods=['POST'])
def update_location():
    return update_location_v1()

@app.route('/get-location', methods=['GET'])
def get_location():
    return jsonify(observer_location)

# Add the missing endpoint that Arduino is expecting
@app.route('/get-server-location', methods=['GET'])
def get_server_location():
    """Endpoint to match the Arduino's request URL"""
    return jsonify(observer_location)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
