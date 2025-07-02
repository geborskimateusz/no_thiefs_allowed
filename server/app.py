from flask import Flask, request, jsonify
import os

app = Flask(__name__)
API_AUTH_TOKEN = os.getenv("API_AUTH_TOKEN")

@app.route('/health', methods=['GET'])
def health():
    auth_header = request.headers.get('Authorization', '')
    if not auth_header.startswith('Bearer ') or auth_header.split(' ')[1] != API_AUTH_TOKEN:
        return jsonify({'error': 'Unauthorized'}), 401
    return jsonify({'status': 'OK'}), 200

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
