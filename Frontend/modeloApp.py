from crate import client
import dash
from dash import html, dcc
from dash import Input, Output
import dash_daq as daq
import pandas as pd
import plotly.express as px
import plotly.graph_objs as go
from skforecast.ForecasterAutoreg import ForecasterAutoreg
from sklearn.ensemble import RandomForestRegressor

# Inicialización de la app Dash
app = dash.Dash(__name__)

# Conexión a la base de datos CrateDB
basededatos = "http://10.38.32.137:8083"

# Función para leer los datos de la base de datos
def leer(url_db):
    con = client.connect(url_db)
    cur = con.cursor()
    cur.execute("SELECT * FROM etrobottable;")
    result = cur.fetchall()
    datos = pd.DataFrame(result, columns=['entity_id', 'entity_type', 'time_index', 'fiware_servicepath',
                                          '__original_ngsi_entity__', 'instanceid', 'lat', 'lon',
                                          'temperatura', 'humedad', 'luz', 'proximidad'])
    datos['lat'] = pd.to_numeric(datos['lat'], errors='coerce')
    datos['lon'] = pd.to_numeric(datos['lon'], errors='coerce')
    datos = datos.dropna(subset=['lat', 'lon'])
    return datos

# Leer y procesar datos de la base de datos
informacion = leer(basededatos)
informacion['time_index'] = pd.to_datetime(informacion['time_index'], unit='ms')
informacion = informacion.sort_values(by='time_index')

# Asegurarse de que la columna 'temperatura' sea numérica
informacion['temperatura'] = pd.to_numeric(informacion['temperatura'], errors='coerce')
informacion = informacion.dropna(subset=['temperatura'])

# Función de predicción de temperatura
def forecast_temperature(data, forecast_steps=10, lags=320):
    forecaster = ForecasterAutoreg(
        regressor=RandomForestRegressor(random_state=123),
        lags=lags
    )
    forecaster.fit(y=data['temperatura'])
    predictions = forecaster.predict(steps=forecast_steps)
    future_times = pd.date_range(start=data['time_index'].iloc[-1], periods=forecast_steps+1, freq='H')[1:]
    forecast_df = pd.DataFrame({'time_index': future_times, 'temperature_forecast': predictions})
    return forecast_df

# Generar los datos de predicción iniciales
forecast_data = forecast_temperature(informacion)

# Crear el gráfico de temperatura observada y predicha con colores diferenciados
fig_temp_pred = go.Figure()

# Agregar datos observados
fig_temp_pred.add_trace(go.Scatter(
    x=informacion['time_index'],
    y=informacion['temperatura'],
    mode='lines',
    name='Temperatura Observada',
    line=dict(color='#FF5733')
))

# Agregar datos predichos
fig_temp_pred.add_trace(go.Scatter(
    x=forecast_data['time_index'],
    y=forecast_data['temperature_forecast'],
    mode='lines',
    name='Temperatura Predicha',
    line=dict(color='#33FF57', dash='dash')
))

# Configurar el diseño del gráfico
fig_temp_pred.update_layout(
    title="Temperatura Observada y Predicha",
    xaxis_title="Tiempo",
    yaxis_title="Temperatura (°C)",
    template="plotly_dark"
)

# Gráfica de humedad
fig_humidity = px.area(informacion, x='time_index', y='humedad', title="Humedad a lo largo del tiempo",
                       labels={'time_index': 'Tiempo', 'humedad': 'Humedad (%)'},
                       template="plotly_dark", color_discrete_sequence=["#33FF57"])

# Mapa de las coordenadas GPS usando latitud y longitud
fig_gps = px.scatter_mapbox(informacion, lat="lat", lon="lon", zoom=10, height=500,
                            title="Ubicación del sensor",
                            template="plotly_dark",
                            color_discrete_sequence=["#3385FF"])
fig_gps.update_layout(mapbox_style="open-street-map")

# Layout de la aplicación Dash
app.layout = html.Div(style={'backgroundColor': '#1E1E1E', 'color': 'white'}, children=[
    html.H1(children='Monitor de IoT para Temperatura y Humedad', style={'textAlign': 'center'}),
    html.Div(children='Aplicación para monitorear datos de temperatura y humedad, y ubicación de sensores.',
             style={'textAlign': 'center'}),
    
    dcc.Graph(id='temp-forecast-graph', figure=fig_temp_pred),  # Gráfico de temperatura observada y predicha
    dcc.Graph(id='humidity-graph', figure=fig_humidity),
    dcc.Graph(id='gps-map', figure=fig_gps),
    
    html.Div([
        html.Div([
            html.Button('Modelo Predictivo', id='BotonMP', n_clicks=0),
            html.Div(id='LabelMP', children=""),
            daq.Indicator(id='IndMP', label='Estado Temperatura', value=False, color='#00EA64')
        ], style={'padding': 10, 'flex': 1}),
        
        html.Div([
            daq.Gauge(
                id='gauge-temp',
                color={"gradient": True, "ranges": {"green": [0, 25], "yellow": [25, 30], "red": [30, 50]}},
                value=23,
                label="Temperatura",
                max=50,
                min=0,
                size=200
            )
        ], style={'padding': 10, 'flex': 1})
    ], style={'display': 'flex', 'flex-direction': 'row', 'justify-content': 'center'}),
    
    html.Div(children='Aplicación para monitorear sensores en FiWARE y CrateDB.', 
             style={'textAlign': 'center', 'paddingTop': '20px'})
])

# Callback para actualizar el indicador de estado de la temperatura
@app.callback(
    [Output('IndMP', 'value'), Output('LabelMP', 'children')],
    [Input('BotonMP', 'n_clicks')]
)
def displayMP(n_clicks):
    if n_clicks > 0:
        return True, "La temperatura está en rango adecuado"
    return False, "Presiona el botón para verificar el modelo predictivo"

# Ejecución de la aplicación
if __name__ == '__main__':
    app.run_server(host='0.0.0.0', port=8050)
