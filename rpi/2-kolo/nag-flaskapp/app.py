import sys
sys.path.append("/home/pi/.local/lib/python3.7/site-packages")
sys.path.append("/home/pi/.local/lib/python3.7/dist-packages")

from flask import Flask
from flask import render_template, request, jsonify
import redis
import json

app = Flask(__name__)

r = redis.Redis(host='localhost', port=6379, db=0)

if "control-strings.json":
    with open("control-strings.json", 'r') as f:
        cs = json.load(f)

@app.route('/', methods=['GET', 'POST'])
def index():
    if request.method == 'POST':
        name = request.form.get("name")
        state = request.form.get("state")

        r.set(name, cs[name][state]["reverse-state"])

    return render_template("index.html")


@app.route('/_refresh_data')
def _refresh_data():
    status = {
        "stairway-light": r.get("stairway-light").decode('utf-8'),
        "garden-light": r.get("garden-light").decode('utf-8'),
        "barrier": r.get("barrier").decode('utf-8'),
        "latest access": r.get("latest access").decode('utf-8'),
        "security-system": r.get("security-system").decode('utf-8'),
        "light-level": float(r.get("light-level")),
        "humidity": float(r.get("humidity")),
        "temperature": float(r.get("temperature")),
    }

    buttons = {}

    for i in cs.keys():
        try:
            buttons[i] = cs[i][status[i]]["button"]
        except KeyError:
            buttons[i] = ""

    return jsonify(status=status, buttons=buttons)


if __name__ == '__main__':
    app.run(host="10.1.2.1")
