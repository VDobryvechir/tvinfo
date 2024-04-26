function oppdaterSkjerm() {
    var area = document.createElement("div");
    area.innerHTML="Ferdig";
    document.body.appendNode(area);
}
function oppdater() {
    var el = document.createElement("button");
    el.innerHTML= "Append code";
    el.addEventListener("click", oppdaterSkjerm);
    document.body.appendNode(el);
}
oppdater();
