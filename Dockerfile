FROM python:3.12-slim AS ascii-assets

WORKDIR /work
RUN apt-get update \
    && apt-get install -y --no-install-recommends ffmpeg \
    && rm -rf /var/lib/apt/lists/* \
    && pip install --no-cache-dir pillow
COPY generator.py /work/generator.py
COPY assets/intro/intro.mp4 /work/assets/intro/intro.mp4
COPY assets/intro/data.json /work/assets/intro/data.json
RUN python /work/generator.py --frame-limit 82 --ascii-dir /work/generated/intro

FROM emscripten/emsdk:3.1.70 AS build

WORKDIR /app
COPY main_web.cpp /app/main_web.cpp
COPY assets/logo /app/assets/logo
COPY assets/fonts /app/assets/fonts
COPY assets/web /app/assets/web
COPY --from=ascii-assets /work/generated/intro /app/assets/intro
RUN bash /app/assets/web/build-web.sh

FROM nginx:1.27-alpine
COPY --from=build /app/dist/ /usr/share/nginx/html/
EXPOSE 80
