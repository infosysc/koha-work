[% USE Koha %]
<script type="text/javascript">
var rfidhuburl = "[% Koha.Preference( 'RFIDHubURL' ) %]";
/*
 * JavaScript to inject Table for RFID items
 * connects to rfid hub with Websocket
 * creates <div> with <table>
 * inserts table to #patronlists and adds highlighted items as they arrive from websocket
 */

$(document).ready(function() {
  var ws=new WebSocket(rfidhuburl);
  var body_id = $("body").attr("id");
  ws.onopen = function() {
    console.log("connected");

    ws.onmessage = function(resp) {
      var data = JSON.parse(resp.data);
      switch (data.Action) {
        case "CONNECT":
          if (data.RFIDError) { // RFIDhun cannot access RFID
            alert("ERROR connecting to RFID, please refresh Browser with F5 to try again!");
          } else if (data.SIPError) { // RFIDhub cannot access SIP Server
            alert("ERROR connecting to SIP Server, please ask Zarathustra for help!");
          } else {
            console.log("Connected to RFIDhub");
            if (body_id == "circ_returns") { 
              ws.send(JSON.stringify({Action: "CHECKIN"})); 
            } else if (body_id == "circ_circulation") {
              ws.send(JSON.stringify({Action: "CHECKOUT", Patron: "[% borrower.borrowernumber %]"})); 
            }
          }
          break;
        case "DISCONNECT":
          console.log("Disconnected from RFIDhub");
          break;
        case "CHECKIN":
          console.log(" <- "+resp.data);
          handleItem(data);
          break;
        case "CHECKOUT":
          // TODO: handle checkout
          console.log(" <- "+resp.data);
          handleItem(data);
          break;
        case "WRITE":
          // TODO: handle write
          console.log(" <- "+resp.data);
          break;
      }
    }
  }
  ws.onclose = function() {
    console.log("disconnected");
  }
  ws.onerror = function() {
    console.log("No connection to RFIDhub!");
    alert("ERROR connecting to RFIDhub, please refresh Browser with F5 to try again!");
  }
});

function handleItem(msg) {
  
  var rfiddiv = getRfidDiv();

  // create table, headers and tbody if not exists
  if ($('#rfiditems').length == 0) {
    var table = $('<table/>', { id: 'rfiditems' });
    table.append('<thead>' +
      '<tr>' +
        '<th scope="col">Dato</th>' +
        '<th scope="col">Tittel</th>' +
        '<th scope="col">Forfatter</th>' +
        '<th scope="col">Lånt ut fra</th>' +
        '<th scope="col">Hyllesignatur</th>' +
        '<th scope="col">Strekkode</th>' +
        '<th scope="col">Beskjed</th>');
    table.append('<tbody/>');

    rfiddiv.append(table);
  }

  // create row
  var row = $('<tr>').attr('id', msg.Item.Barcode)
    .append('<td><span title="dato">'+msg.Item.Date+'</span></td>')
    .append('<td><strong>'+msg.Item.Label+'</strong></a></td>') 
    .append($('<td>'))
    .append($('<td>'))
    .append($('<td>'))
    .append('<td><a href="/cgi-bin/koha/catalogue/detail.pl?biblionumber='+
        msg.Item.Barcode+'&amp;type=intra">'+msg.Item.Barcode+'</a></td>')
    .append('<td>'+msg.Item.Status);

  // if row with id already exists: update - else prepend
  if($("#" + msg.Item.Barcode).length == 0) {
    rfiddiv.find('tbody').prepend(row);
  } else {
    rfiddiv.find($("#" + msg.Item.Barcode).parent('tr')).replaceWith(row);
  }

  // highlight according to result 
  if(msg.Item.OK) {
    row.children('td').effect("highlight", {color: "lightgreen"}, 3000);
  } else {
    row.children('td').css("backgroundColor", "pink");
  }
}

function getRfidDiv() {
  // Create div with id rfidlist if not exists
  if ($('#rfidlist').length == 0) {
    var rfiddiv = $('<div/>', {
      id: 'rfidlist',
      class: 'toptabs ui-tabs ui-widget ui-widget-content ui-corner-all'
    }).prependTo('#patronlists');
  } else {
    var rfiddiv = $('#rfidlist');
  }
  return rfiddiv
}

</script>
