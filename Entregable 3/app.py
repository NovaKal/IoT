from dash import Dash, Input, Output, dcc, html
from scipy.interpolate import griddata
from urllib.request import urlopen
from flask import Flask, request
import dash_bootstrap_components as dbc
import plotly.graph_objects as go
import pandas as pd
import numpy as np
import json


# Initialize app

app = Dash(
    __name__,
    external_stylesheets = [dbc.themes.BOOTSTRAP]
)

app.title = "UPB IoT 2022 - 10"
server = app.server
token = "pk.eyJ1Ijoibm92YS1hbHBoYSIsImEiOiJjbDJjbm1zaDUwbjA4M3BwN3RlbTU5NjJvIn0.UIWUPCVh5RAadbmn8yqPfA"

# Load data

def load_data():
    url = "http://siata.gov.co:8089/estacionesAirePM25/cf7bb09b4d7d859a2840e22c3f3a9a8039917cc3/"
    responde_url = urlopen(url)
    data = json.loads(responde_url.read())
    df = pd.json_normalize(
        data,
        record_path = ['datos'])

    latitud = []
    longitud = []
    for i in range(0, 18):
        latitud.append(df['coordenadas'][i][0]['latitud'])
        longitud.append(df['coordenadas'][i][0]['longitud'])

    df['latitud'] = latitud
    df['longitud'] = longitud

    # Cleaning dataframe

    df = df.drop('urlIcono', axis = 1)
    df = df.drop('coordenadas', axis = 1)
    df = df.drop('unidadesValorICA', axis = 1)

    index_errors = df[df['valorICA'] == -9999.0].index
    df.drop(index_errors, inplace = True)
    return df

# ICA Calculus

def ICA_Calculus(df, latitud, longitud):

    x_inferior = df["longitud"].min()
    x_superior = df["longitud"].max()
    y_inferior = df["latitud"].min()
    y_superior = df["latitud"].max()

    xi = np.linspace(x_inferior, x_superior, 100)
    yi = np.linspace(y_inferior, y_superior, 100)

    ica_value = griddata((df["latitud"], df["longitud"]), df["valorICA"], (latitud, longitud), method = "cubic")

    return ica_value

# AQI Values

def ICA_to_AQI(ica_value):

    recomendacion = ""

    if 0 <= ica_value < 12:
        aqi_value = AQI_Calculus(0, 50, 0, 12, ica_value)
        recomendacion = "La calidad del aire se considera buena (Puede haber muy poco o ningún riesgo)."
    elif 12.1 <= ica_value < 35.4:
        aqi_value = AQI_Calculus(51, 100, 12.1, 35.4, ica_value)
        recomendacion = "Las personas usualmente sensibles deben de considerar reducir la exposición prolongada."
    elif 35.5 <= ica_value < 55.4:
        aqi_value = AQI_Calculus(101, 150, 35.5, 55.4, ica_value)
        recomendacion = "Las personas con enfermedades cardiacas, pulmonares, adultos mayores y niños deben de considerar reducir la exposición prolongada."
    elif 55.5 <= ica_value < 150.4:
        aqi_value = AQI_Calculus(151, 200, 55.5, 150.4, ica_value)
        recomendacion = "Las personas con enfermedades cardiacas, pulmonares, adultos mayores y niños deben reducir la exposición prolongada y el resto de personas deben de considerar reducir la exposición prolongada."
    elif 150.5 <= ica_value < 250.4:
        aqi_value = AQI_Calculus(201, 300, 150.5, 250.5, ica_value)
        recomendacion = "Todas las personas deben de reducir la exposición."
    elif 250.5 <= ica_value < 500:
        aqi_value = AQI_Calculus(301, 500, 250.5, 500.4, ica_value)
        recomendacion = "Alerta sanitaria."
    return aqi_value, recomendacion

# AQI Calculus

def AQI_Calculus(aqi_lo, aqi_hi, bp_lo, bp_hi, cp):
    aqi_value = (aqi_hi - aqi_lo) * (cp - bp_lo) / (bp_hi - bp_lo) + aqi_lo
    return aqi_value

# Heatmap

def figure_heatmap(df_1):
    
    x_inferior = df_1["longitud"].min()
    x_superior = df_1["longitud"].max()
    y_inferior = df_1["latitud"].min()
    y_superior = df_1["latitud"].max()

    xi = np.linspace(x_inferior, x_superior, 100)
    yi = np.linspace(y_inferior, y_superior, 100)
    grid_x, grid_y = np.meshgrid(xi, yi)

    grid_z0 = griddata((df_1["latitud"], df_1["longitud"]), df_1["valorICA"], (grid_y, grid_x), method='nearest')
    grid_z2 = griddata((df_1["latitud"], df_1["longitud"]), df_1["valorICA"], (grid_y, grid_x), method='cubic')

    rows = grid_z0.shape[0]
    cols = grid_z0.shape[1]

    for x in range(0, cols - 1):
        for y in range(0, rows - 1):
            if np.isnan(grid_z2[x,y]):
                grid_z2[x,y] = grid_z0[x,y]

    lat = []
    lon = []
    z = []

    df_2 = pd.DataFrame()
    for x in range(0, cols - 1):
        for y in range(0, rows - 1):
            lat.append(grid_y[x,y])
            lon.append(grid_x[x,y])
            z.append(grid_z2[x,y])

    d = {"lat": lat, "lon": lon, "value": z}
    df_2 = pd.DataFrame(data = d)

    fig = go.Figure(
        go.Densitymapbox(
            lat = df_2.lat,
            lon = df_2.lon,
            z = df_2.value,
            zmin = 1,
            zmax = 30,
            radius = 20,
            opacity = 0.8
        )
    )

    fig.update_layout(
        margin = dict(r = 0, l = 0, b = 0, t = 0),
        mapbox = dict(
            accesstoken = token,
            style = "stamen-terrain",
            zoom = 10,
            center = dict(
                lat = 6.1856666,
                lon = -75.5972061
            )
        ),
        showlegend = True,
        autosize = True
    )
    return fig

# Scattermapbox points

def figure_points(df):
    fig = go.Figure(go.Scattermapbox(
        lat = df["latitud"],
        lon = df["longitud"],
        mode = "markers+text",
        marker = go.scattermapbox.Marker(
            size = df["valorICA"] * 1.4,
            color = df["colorIconoHex"],
            colorscale = "Edge",
            showscale = True,
            sizemode = "diameter",
            opacity = 0.8
        ),
        hoverinfo = "text",
        hovertext = "<b>Nombre</b>: " + df["nombre"].astype(str) + "<br>" +
        "<b>Valor ICA</b>: " + df["valorICA"].astype(str) + "<br>" +
        "<b>Latitud</b>: " + df["latitud"].astype(str) +
        "<b> Longitud</b>: " + df["longitud"].astype(str) + "<br>"
    ))

    fig.update_layout(
        hovermode = "x",
        margin = dict(r = 0, l = 0, b = 0, t = 0),
        mapbox = dict(
            accesstoken = token,
            style = "stamen-terrain",
            zoom = 10,
            center = dict(
                lat = 6.1856666,
                lon = -75.5972061
            )
        ),
        showlegend = True,
        autosize = True
    )

    return fig

# App Layout

SIDEBAR_STYLE = {
    "position": "fixed",
    "top": 0,
    "left": 0,
    "bottom": 0,
    "width": "16rem",
    "padding": "2rem 1rem",
    "background-color": "#f8f9fa",
}

CONTENT_STYLE = {
    "margin-left": "18rem",
    "margin-right": "2rem",
    "padding": "2rem 1rem",
}

sidebar = html.Div(
    [
        html.H2("Menu", className = "display-4"),
        html.Hr(),
        html.P(
            "UPB - Internet Of Things 2022 - 10 Kevin Alejandro Vasco Hurtado", className = "lead"
        ),
        dbc.Nav(
            [
                dbc.NavLink("AQI Recomendation", href = "/", active = "exact"),
                dbc.NavLink("HM Map", href = "/map_heatmap", active = "exact"),
                dbc.NavLink("SB Map", href = "/map_scatterbox", active = "exact"),
            ],
            vertical = True,
            pills = True,
        ),
    ],
    style = SIDEBAR_STYLE,
)

#

toast = html.Div(
    [
        dbc.Button(
            "Recomendation",
            id = "positioned-toast-toggle",
            color = "primary",
            n_clicks = 0,
        ),
        dbc.Toast(
            [
                html.P("La monda de tarzan  ")
            ],
            id = "positioned-toast",
            header = "Recomendation",
            is_open = False,
            dismissable = True,
            icon = "danger",
            # top: 66 positions the toast below the navbar
            style = {"position": "fixed", "top": 66, "right": 10, "width": 350},
        ),
    ]
)

#

latitud_input = html.Div(
    [
        dbc.Label(
            "Latitud",
            html_for = "example-latitud"
            ),
        dbc.Input(
            type = "text",
            id = "id_latitud_text",
            placeholder = "Enter Latitud"
            ),
        dbc.FormText(
            "Why are you running?",
            color = "secondary",
        ),
    ],
    className="mb-3",
)

longitud_input = html.Div(
    [
        dbc.Label(
            "Longitud",
            html_for = "example-longitud"
            ),
        dbc.Input(
            type = "text",
            id = "id_longitud_text",
            placeholder = "Enter Longitud",
        ),
        dbc.FormText(
            "Everybody wants to be my enemy",
            color = "secondary"
        ),
    ],
    className = "mb-3",
)

form = dbc.Form([latitud_input, longitud_input])

#

content = html.Div(id = "page-content", style = CONTENT_STYLE)

app.layout = html.Div(
    [
        dcc.Location(id = "url"),
        sidebar,
        content
    ]
)

@app.callback(
    Output(
        "page-content",
        "children"
    ),
    [Input(
        "url",
        "pathname"
    )]
)
def render_page_content(pathname):
    if pathname == "/":
        return form, toast
    elif pathname == "/map_heatmap":
        return dcc.Graph(figure = figure_heatmap(load_data()))
    elif pathname == "/map_scatterbox":
        df = load_data()
        return dcc.Graph(figure = figure_points(load_data()))
    # If the user tries to reach a different page, return a 404 message
    return dbc.Jumbotron(
        [
            html.H1("404: Not found", className="text-danger"),
            html.Hr(),
            html.P(f"The pathname {pathname} was not recognised..."),
        ]
    )

@app.callback(
    Output("positioned-toast", "is_open"),
    Output("positioned-toast", "children"),
    Input("positioned-toast-toggle", "n_clicks"),
    Input("id_latitud_text", "value"),
    Input("id_longitud_text", "value"),
)
def open_toast(n, latitud, longitud):
    if n:
        aqi_value, recomendacion = ICA_to_AQI(ICA_Calculus(load_data(), latitud, longitud))
        print(aqi_value)
        print(recomendacion)
        return ["monda", recomendacion]
    return ["monda", recomendacion]

if __name__ == "__main__":
    app.run_server(host = "0.0.0.0", port = 80)

#Run Docker Command with Dockerfile:  sudo docker build -t server:iot1 . <-- El punto
#Run Docker Command manual: sudo docker run -d -p 80:80 server:iot1 python3 /home/app.py