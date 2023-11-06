const net = require("net");

if (process.argv.length !== 3) {
  console.log("Usage: node tcp_server.js <port>");
  process.exit(1);
}

const port = parseInt(process.argv[2]);

const server = net.createServer((socket) => {
  console.log("Client connected.");

  socket.on("data", (data) => {
    console.log(`Received data from client: ${data.toString()}`);
    socket.write("Hello, MOZERFOKER!\n");
  });

  socket.on("end", () => {
    console.log("Client disconnected.");
  });
});

server.listen(port, () => {
  console.log(`Server is listening on port ${port}.`);
});

server.on("error", (err) => {
  console.error(`Server error: ${err}`);
});

server.on("close", () => {
  console.log("Server closed.");
});
