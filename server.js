var net = require('net');

const SERVER_PORT = 6000;
const SERVER_ADRESS ="0.0.0.0"

const NUM_HOSTS = 10

var receivedData = Array(NUM_HOSTS).fill("");

for (var i = SERVER_PORT; i < SERVER_PORT + NUM_HOSTS; i++)
    ((i)=>
    net.createServer((socket) => {
    socket.on('data', (data) => {
        receivedData[i] = receivedData[i] + data
        while (receivedData[i].indexOf("\n") >= 0) {
            if (receivedData[i].startsWith("POST /") || receivedData[i].startsWith("GET /")) {
                var req = receivedData[i].substring(0, receivedData[i].indexOf("\n"));
                socket.write(processRequest(req.split('\t\t')[0].split(" ")[0], req.split('\t\t')[0].split(" ")[1], req.split('\t\t').slice(1).join('\t\t')) + "\n")
            }
            receivedData[i] = receivedData[i].substring(receivedData[i].indexOf("\n") + 1, receivedData[i].length)
        }
    });
    }).listen(i, SERVER_ADRESS,() => {
        console.log("[SERVER] started on port " + i)
    }))(i)


var processRequest = function(method, path, body) {
    console.log(`[SERVER] received ${method} at ${path} with '${body}'`)

    if (method == "POST" && path == "/diagnosis-keys") return addDiagnosisKeys(body)
    if (method == "GET" && path == "/diagnosis-keys") return sendDiagnosisKeys() 
    return "";
}

var diagnosisKeys = []

var addDiagnosisKeys = function (keys) {
    diagnosisKeys = diagnosisKeys.concat(keys.split(","))
    return "SUCCESS";
}

var sendDiagnosisKeys = function (keys) {
    return diagnosisKeys.join("");
}