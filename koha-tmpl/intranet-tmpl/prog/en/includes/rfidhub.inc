[% USE Koha %]
<link rel="stylesheet" type="text/css" href="[% interface %]/lib/bootstrap/bootstrap.min.css" />
<script type="text/javascript" src="[% interface %]/lib/bootstrap/bootstrap.min.js"></script>
<script type="text/javascript">
var rfidhuburl = "[% Koha.Preference( 'RFIDHubURL' ) %]";
/*
 * JavaScript to inject Table for RFID items
 * connects to rfid hub with Websocket
 * creates <div> with <table>
 * inserts table to #patronlists and adds highlighted items as they arrive from websocket
 */

$(document).ready(function() {
  ws=new WebSocket(rfidhuburl);
  var body_id = $("body").attr("id");
  // activate RFID Modal
  $("#menu ul").append('<li><a href="#" onclick="activateWrite();">RFID</a></li>');
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
              ws.send(JSON.stringify({Action: "CHECKOUT", Patron: "[% borrower.cardnumber %]"})); 
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
          console.log(" <- "+resp.data);
          handleItem(data);
          break;
        case "ITEM-INFO":
          console.log(" <- "+resp.data);
          displayItemInfo(data.Item)
          break;
        case "WRITE":
          console.log(" <- "+resp.data);
          $('#itemStatusDiv').toggleClass('hidden', false);
          if (data.RFIDError) { // Failed to Write
            $('#itemStatus').text("ERROR writing tag, try again or call it a day!");
            $('#itemStatus').attr("style", "background-color: pink");
          } else {
            $('#itemStatus').text("SUCCESS writing tag!");
            $('#itemStatus').attr("style", "background-color: lightgreen");
          }
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
    rfiddiv.find($("#" + msg.Item.Barcode).closest('tr')).replaceWith(row);
  }

  // highlight according to success or failure
  if(msg.Item.OK) {
    row.children('td').effect("highlight", {color: "lightgreen"}, 3000);
  } else {
    row.children('td').css("backgroundColor", "pink");
  }
}


function activateWrite() {
  buildWriteRFIDModal();
  $("#writeRFIDModal").modal('show');
}

function buildWriteRFIDModal() {
  var writeRFIDModal = getWriteRFIDModal();
  writeRFIDModal.html('<div class="modal-header">'+
  '<button type="button" class="close" data-dismiss="modal" aria-hidden="true">×</button>'+
  '<h3 id="writeRFID">Write RFID Tag</h3></div>'+
  '<div class="modal-body">'+
  '<p><input id="barcodeInput" placeholder="barcode here" class="input-xlarge"></p>'+
  '<div id="itemInfoDiv" class="hidden">'+
  '<p><input id="itemLabel" type="text" class="input-xlarge" disabled="disabled"></p>'+
  '<p><input id="itemTags" type="number" ></p>'+
  '</div>'+
  '<div id="itemStatusDiv" class="hidden"></div>'+
  '<p id="itemStatus">&nbsp;</p>'+
  '</div>'+
  '<div class="modal-footer">'+
  '<a href="#" id="writeButton" class="hidden btn btn-danger" onclick="return writeItem();">preg brikker</a>'+
  '<a href="#" class="btn btn-success" onclick="return lookupItem();">søk opp bok</a></div>');
}

function lookupItem() {
  // End session and request item info
  ws.send(JSON.stringify({Action: "END" }));
  ws.send(JSON.stringify({Action: "ITEM-INFO", Item: { Barcode: $('#barcodeInput').val() } }));
  return true;
}

function writeItem() {
  var msg = JSON.stringify({Action: "WRITE", Item: { Barcode: $('#barcodeInput').val(), NumTags: parseInt($('#itemTags').val()) } });
  console.log(msg);
  ws.send(msg);
  return true;
}

function displayItemInfo(item) {
  console.log(item);
  $("#itemLabel").attr("value", item.Label);
  $("#itemTags").attr("value", item.NumTags);
  $("#itemInfoDiv").toggleClass('hidden', false);
  $("#writeButton").toggleClass('hidden', false);
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
  return rfiddiv;
}

function getWriteRFIDModal() {
  // Create modal div with id writeRFIDModal if not exists
  if ($('#writeRFIDModal').length == 0) {
    var writeRFIDModal = $('<div/>', {
      id: 'writeRFIDModal',
      class: 'modal hide fade',
      tabindex: '-1',
      role: 'dialog',
      "aria-labelledby": 'writeRFID',
      "aria-hidden": true
    }).prependTo('#rfidlist');
  } else {
    var writeRFIDModal = $('#writeRFIDModal');
  }
  return writeRFIDModal;
}

</script>
