from flask import Flask, request, render_template
from itertools import cycle
from OpenSSL import SSL
import json
import xxhash
import sqlite3

db_data = 'database/sensor_data.db'
db_signature = 'database/signature.db'
app = Flask(__name__)

@app.route('/')
def home():
    return render_template('index.html')

@app.route('/registro')
def registro():
    return render_template('registro.html')

@app.route('/borrar_db')
def borrardb():
    create_command = '''CREATE TABLE data (
        identifier NUMERIC,
        sequence NUMERIC,
        timestamp DATETIME,
        latitud NUMERIC,
        longitud NUMERIC,
        altitud NUMERIC,
        temperature NUMERIC,
        humidity NUMERIC,
        lux NUMERIC
        )'''

    con = sqlite3.connect(db_data)
    cur = con.cursor()
    cur.execute("DROP TABLE IF EXISTS data")
    cur.execute(create_command)
    con.commit()
    con.close()
    return "ok", 201

@app.route('/sensor_data', methods=['POST'])
def sensor_send():
    key = "https://www.youtube.com/watch?v=eshyEOsRZnM"
    request_data = request.get_json()
    data_str     = request_data['data']
    checksum     = request_data['checksum']
    signature    = request_data['signature']

    hashObject   = xxhash.xxh32(data_str)
    digest       = hashObject.hexdigest()
    digest       = digest.upper()

    if (digest == checksum):
        #res = str_xor(checksum, key)
        if (True):
            data = json.loads(data_str)
            identifier  = data['identifier']
            sequence    = data['sequence']
            timestamp   = data['timestamp'].split("_")[0]
            latitud     = data['latitud']
            longitud    = data['longitud']
            altitud     = data['altitud']
            temperature = data['temperature']
            humidity    = data['humidity']
            lux         = data['lux']

            insert_command = "INSERT INTO data VALUES (" + identifier + "," + sequence + "," + timestamp + "," + latitud + "," + longitud + "," + altitud + "," + temperature + "," + humidity + "," + lux + ")"

            con = sqlite3.connect(db_data)
            curs = con.cursor()
            curs.execute(insert_command)
            curs.close()
            con.close()

    return "ok", 201


def str_xor(s1, s2):
    return "".join([chr(ord(c1) ^ ord(c2)) for (c1,c2) in zip(s1,s2)])

@app.route("/visualization")
def captura():
    identifier_array  = []
    sequence_array    = []
    timestam_array    = []
    lat_array         = []
    long_array        = []
    alt_array         = []
    temperature_array = []
    humidity_array    = []
    lux_array         = []

    con = sqlite3.connect(db_data)
    curs = con.cursor()
    for fila in curs.execute("SELECT * FROM data"):
        identifier_array.append(fila[0])
        sequence_array.append(fila[1])
        timestam_array.append(fila[1])
        lat_array.append(fila[2])
        long_array.append(fila[3])
        alt_array.append(fila[4])
        temperature_array.append(fila[5])
        humidity_array.append(fila[6])
        lux_array.append(fila[7])
    con.close()
    print(timestam_array)
    leyendas = ['Temperatura ensor','Humedad Sensor', 'Lux Sensor']
    etiquetas = timestam_array
    return render_template('chart.html', humidity = humidity_array, temperature = temperature_array, lux = lux_array, labels = etiquetas, legend = leyendas)

if __name__ == '__main__':
    app.run(host = '0.0.0.0', port = 443, ssl_context = ('cert.crt', 'key.pem'))