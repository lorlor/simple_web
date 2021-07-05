var ws;         // WebSocket对象
var wss_url = "ws://localhost:9002/";   // WebSocket通信地址
var video_stream_start = "command:video_start";
var video_stream_stop = "command:video_stop";

ws = new WebSocket(wss_url);

// Open callback
ws.onopen = function()
{
    console.log("Open WS connction");
    // ws.send("hello");
}

// Message callback
ws.onmessage = function(evt)
{
    data_json = JSON.parse(evt.data);
    if(data_json.command == "video_ready"){
        ws.send("command:video_tick");
    }

    if(data_json.command == "image"){
        arraybuffer = evt.data;
        var img_bytes = new Uint8Array(arraybuffer);

        var canvas = document.getElementById("video_disp");
        var ctx = canvas.getContext("2d");
        var img = new Image();
        // console.log(window.btoa(img_bytes));
        img.onload = function(){
            // console.log("img w: ", img.width);
            // console.log("img h: ", img.height);
            ctx.drawImage(img, 0, 0, img.width, img.height, 0, 0, canvas.width, canvas.height);
        }

        img.src = "data:image/jpeg;base64," + data_json.payload; // 如果使用基于C++ websocketpp库的websocket server传输图像，需要这句话

        // 接收到图像之后，延迟一定的时间再接收下一帧
        setTimeout(function(){
            if(img_recv_flag)
                ws.send("command:video_tick");
        }, 30);
    }
}

// Close callback
ws.onclose = function()
{
    ws.close();
}

function btnConnect()
{
    ws.send(video_stream_start);

    ws.binaryType = "arraybuffer";
    var btn = document.getElementById("btnvideoconnect");
    btn.disabled = true;
    img_recv_flag = true;
}

function btnDisconnect()
{
    var canvas = document.getElementById("video_disp");
    var ctx = canvas.getContext("2d");
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    ws.send(video_stream_stop);
    // ws.close();
    var btn = document.getElementById("btnvideoconnect");
    btn.disabled = false;
    img_recv_flag = false;
}
