-- Fase 4 - Sistema domótico: esquema PostgreSQL
-- Crear primero la base:  CREATE DATABASE domotica;
-- Luego ejecutar este archivo conectado a "domotica".

CREATE TABLE IF NOT EXISTS lecturas (
  id          BIGSERIAL PRIMARY KEY,
  ts          TIMESTAMPTZ NOT NULL DEFAULT now(),
  dispositivo TEXT NOT NULL DEFAULT 'casa1',
  sensor      TEXT NOT NULL,
  valor       DOUBLE PRECISION NOT NULL,
  unidad      TEXT
);
CREATE INDEX IF NOT EXISTS idx_lecturas_sensor_ts ON lecturas (sensor, ts DESC);

CREATE TABLE IF NOT EXISTS estado_actuadores (
  actuador    TEXT PRIMARY KEY,
  estado      TEXT NOT NULL,
  actualizado TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE TABLE IF NOT EXISTS alarmas (
  id      BIGSERIAL PRIMARY KEY,
  ts      TIMESTAMPTZ NOT NULL DEFAULT now(),
  tipo    TEXT NOT NULL,
  sensor  TEXT,
  valor   DOUBLE PRECISION,
  umbral  DOUBLE PRECISION,
  mensaje TEXT
);

CREATE TABLE IF NOT EXISTS comandos (
  id       BIGSERIAL PRIMARY KEY,
  ts       TIMESTAMPTZ NOT NULL DEFAULT now(),
  actuador TEXT NOT NULL,
  comando  TEXT NOT NULL
);
