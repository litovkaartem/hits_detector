from flask import Flask, render_template, request
import serial
from threading import Lock

app = Flask(__name__)
ser = serial.Serial('/dev/ttyUSB0', 9600, timeout=1)  # Убедитесь, что это правильный порт
data_lock = Lock()
counters = {}

@app.route('/')
def index():
    with data_lock:
        return render_template('index.html', counters=counters)

@app.route('/reset', methods=['POST'])
def reset():
    device_id = request.form.get('device_id')
    ser.write(b"reset\n")  # Отправляем команду сброса
    with data_lock:
        if device_id in counters:
            counters[device_id] = 0
    return '', 204

def read_serial():
    while True:
        line = ser.readline().decode('utf-8').strip()
        if line:
            try:
                parts = line.split(':')
                if len(parts) == 3:  # ID:count:value
                    device_id, count, value = parts
                    with data_lock:
                        counters[device_id] = int(count)
            except Exception as e:
                print(f"Error processing data: {e}")

if __name__ == '__main__':
    from threading import Thread
    Thread(target=read_serial, daemon=True).start()
    app.run(host='0.0.0.0', port=5000)
