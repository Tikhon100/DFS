FROM node:18

WORKDIR /client

COPY package*.json ./

RUN npm install
RUN npm install -g http-server
RUN npm install vite@5.2.10 @vitejs/plugin-vue@5.0.4 esbuild@0.20.2

COPY . .

RUN npm run build

EXPOSE 8081

CMD ["http-server", "dist", "-p", "8081"]
