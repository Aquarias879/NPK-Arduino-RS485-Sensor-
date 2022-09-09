import re
from typing import NamedTuple

import paho.mqtt.client as mqtt
from influxdb import InfluxDBClient
import datetime

DB_USER = 'sam'
DB_PASSWORD = '12345678'
DATABASE_NAME = 'SensorData'
IP_ADDRESS = '203.64.131.98'
MQ_USER = 'admin'
MQ_PASSWORD = '12345678'
TOPIC = 'greenhouse/+/+'
REGEX = 'greenhouse/([^/]+)/([^/]+)'

influxdb_client = InfluxDBClient(DB_ADDRESS, 8086, DB_USER, DB_PASSWORD, None)

class SensorData(NamedTuple):
    device : str
    measurement: str
    value: float

def on_connect(client, userdata, flags, rc):
    print('Connected with result code ' + str(rc))
    client.subscribe(TOPIC)

def _parse_mqtt_message(topic, payload):
    match = re.match(REGEX, topic)
    if match:
        device = match.group(1)
        measurement = match.group(2)
        if measurement == 'status':
            return None
        return SensorData(device,measurement, float(payload))
    else:
        return None

def _send_sensor_data_to_influxdb(sensor_data):
    json_body = [
        {
            'measurement': sensor_data.measurement,
            'tags': {
                'device': sensor_data.device},
            'fields': {
                'value': sensor_data.value
            }
        }
    ]
    influxdb_client.write_points(json_body)

def on_message(client, userdata, msg):
    print(msg.topic + ' ' + str(msg.payload))
    sensor_data = _parse_mqtt_message(msg.topic, msg.payload.decode('utf-8'))
    if sensor_data is not None:
        _send_sensor_data_to_influxdb(sensor_data)

def _init_influxdb_database():
    databases = influxdb_client.get_list_database()
    if len(list(filter(lambda x: x['name'] == DATABASE_NAME, databases))) == 0:
        influxdb_client.create_database(DATABASE_NAME)
    influxdb_client.switch_database(DATABASE_NAME)

def main():
    _init_influxdb_database()

    mqtt_client = mqtt.Client()
    mqtt_client.username_pw_set(MQ_USER, MQ_PASSWORD)
    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message

    mqtt_client.connect(ADDRESS, 1883)
    mqtt_client.loop_forever()


if __name__ == '__main__':
    print('Finished !!')
    main()

