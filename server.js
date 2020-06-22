var net = require('net');

const SERVER_PORT = 60000;
const CLIENT_PORT = 60001;

var server = net.createServer((socket) => {
    socket.on('data', (data) => {
        console.log(`[SERVER] received '${data.toString()}', echoing`)
        socket.write("echo '"+data.toString()+"'")
    });
}).listen(SERVER_PORT,()=> {
    console.log("[SERVER] started on port " + SERVER_PORT)
});

setInterval(() => {
    if (client) client.destroy();
    try {
        var client = new net.Socket();
        client.connect(CLIENT_PORT, function () {
            console.log('[CLIENT] connected, sending data');
            client.write('Hello, server! Love, Client.');
        });
        client.on('data', function (data) {
            console.log('[CLIENT] Received: ' + data);
            client.destroy();
        });
        client.on('error', function (ex) {
            console.log("[CLIENT] error, couldnt connect to port " + CLIENT_PORT);
        });
    } catch (e) {
        console.log("snap")
    }

}, 5000)
