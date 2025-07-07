from flask import Flask, request, jsonify
from flask_socketio import SocketIO
from flask_cors import CORS

app = Flask(__name__)
CORS(app)
socketio = SocketIO(app, cors_allowed_origins="*")

@app.route('/api/health', methods=['POST'])
def health_check():
    data = request.get_json()
    event = {
        "message": data.get("message", "Event Received")
    }
    socketio.emit('health_event', event)
    return jsonify({"status": "received"}), 200

if __name__ == '__main__':
    socketio.run(app, host="0.0.0.0", port=5000)


