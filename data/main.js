let passwordModal;
let networkManager;
let menuManager;
let curTime = -1;

function init(){
	passwordModal = new Modal(document.getElementById("pwdModalContainer"));
	networkManager = new NetworkManager(document.getElementById("networkEntryContainer"));
	menuManager = new MenuManager("inactive");

	const inputAP = document.getElementById("inputCreateAP");
	const inputStatic = document.getElementById("inputStaticIP");

	inputAP.addEventListener("change", updateNetworkContainer);
	inputStatic.addEventListener("change", updateStaticContainer);

	const changeEv = new Event("change");
	inputAP.dispatchEvent(changeEv);
	inputStatic.dispatchEvent(changeEv);

	setInterval(async () => { // A quick and dirty way of showing the time remaining
		if(curTime !== -1){ // lamp hasn't been started
			if(--curTime === 0){ // lamp is doner kebab, verify and update accordingly
				menuManager.setMenu(await updateLampState());
			}

			updateRemainingTime(curTime);
		}
	}, 1000);

	document.getElementById("btnStart").addEventListener("click", () => {
		startLamp(parseInt(document.getElementById("inputTime").value));
	});
	document.getElementById("btnStop").addEventListener("click", stopLamp);

	document.getElementById("btnSettings").addEventListener("click", settingsBtnHandler);
	document.getElementById("btnApplySettings").addEventListener("click", updateNetworkSettings);
}

class NetworkManager{
	container = null;
	loader = null;
	ssid = null;
	pass = null;
	connectedSSID = null;

	constructor(container){
		this.container = container;
		this.loader = document.getElementById("networkLoading");
		this.connectedSSID = document.getElementById("infoIP").textContent; // Jank AF, but I can't be arsed to spend the 30 seconds coming up with a better way. Instead I can spend 30 seconds writing this comment explaining that I don't want to spend 30 seconds coming up with a better way.
	}

	async loadEntries(){
		this.loader.style.display = "block";
		this.container.innerHTML = "";

		const req = await fetch("/networks", { method: "POST" });
		const data = await req.json();

		console.log(data);

		this.loader.style.display = "none";

		for(const entry of data){
			this.addEntry(entry);
		}
	}

	addEntry(data){ // {ssid, rssi}
		const entry = document.createElement("div");
		entry.className = "networkEntry";
		this.container.appendChild(entry);

		if(data.ssid === this.connectedSSID) entry.setAttribute("data-selected", "data-selected");

		const ssid = document.createElement("span");
		ssid.textContent = data.ssid;
		entry.appendChild(ssid);

		const strength = document.createElement("span");
		strength.textContent = `${data.rssi}db`;
		entry.appendChild(strength);
		
		const btn = document.createElement("button");
		btn.textContent = "Select";
		btn.className = "button";
		btn.setAttribute("data-ssid", data.ssid);
		entry.appendChild(btn);

		btn.addEventListener("click", (e) => { // In a callback to preserve `this`
			this.connectBtnHandler(e);
		});
	}

	connectBtnHandler(e){
		let entryContainer = e.target.parentElement;

		if(entryContainer.hasAttribute("data-selected")) return;

		passwordModal.cbOk = (pass) => {
			this.ssid = e.target.getAttribute("data-ssid");
			this.pass = pass;

			const prevSelected = this.container.querySelector("[data-selected]");

			if(prevSelected) prevSelected.removeAttribute("data-selected");

			entryContainer.setAttribute("data-selected", "data-selected");
		};

		passwordModal.open();
	}

	getSelected(){
		if(this.ssid === null) return null;
		
		return {ssid: this.ssid, pass: this.pass};
	}
}

class Modal{
	container = null;
	input = null;
	okBtn = null;
	cancelBtn = null;
	cbOk = null;
	cbCancel = null;

	constructor(container, cbOk = () => {}, cbCancel = () => {}){
		this.container = container;
		this.input = container.querySelector("[data-input]");
		this.okBtn = container.querySelector("[data-ok]");
		this.cancelBtn = container.querySelector("[data-cancel]");

		this.cbOk = cbOk;
		this.cbCancel = cbCancel;

		this.okBtn.addEventListener("click", () => {
			this.ok();
		});

		this.cancelBtn.addEventListener("click", () => {
			this.cancel();
		});
	}

	open(){
		this.input.value = "";
		this.container.style.display = "grid";
	}

	hide(){
		this.container.style.display = "none";
	}

	ok(){
		this.cbOk(this.input.value);
		this.hide();
	}

	cancel(){
		this.cbCancel();
		this.hide();
	}
}

class MenuManager{
	menu = null;
	
	constructor(baseMenu = ""){
		this.menu = baseMenu;

		this.update();
	}

	update(){
		// Hide all active menus (Just in case there are multiple for some odd reason)
		for(const el of document.querySelectorAll(`[data-menuActive]`)){
			el.removeAttribute("data-menuActive"); 
		}

		document.querySelector(`[data-menu="${this.menu}"]`).setAttribute("data-menuActive", "data-menuActive");
	}

	setMenu(newMenu){
		this.menu = newMenu;
		this.update();
	}
}

async function startLamp(time){
	const startBtn = document.getElementById("btnStart");
	startBtn.disabled = true; // Disable the button to prevent two presses

	await fetch("/start", { method: "POST", body: `time=${time.toString()}` });

	menuManager.setMenu("active");
	updateRemainingTime(time); // Set the time to the real time

	startBtn.disabled = false;
}

async function stopLamp(){
	const stopBtn = document.getElementById("btnStop");
	stopBtn.disabled = true; // Disable the button to prevent two presses

	const req = await fetch("/stop", { method: "POST" });

	menuManager.setMenu("inactive");
	curTime = -1;
	stopBtn.disabled = false;
}

async function updateLampState(){
	const req = await fetch("/getState", { method: "POST" });
	const data = await req.json(); // {state, time} (time only present if state == active)

	if(data.state === "active") updateRemainingTime(data.time); 

	return data.state;
}

function updateRemainingTime(time){
	curTime = time;
	const sec = (time % 60).toString();
	document.getElementById("timeRemaining").textContent = `${Math.floor(time / 60)}:${sec.length === 1 ? `0${sec}` : sec}`;
}

function updateNetworkContainer(e){
	const container = document.getElementById("networkSettingsContainer");

	if(e.target.checked){
		container.style.display = "none";
	}
	else{
		container.style.display = "block";
		networkManager.loadEntries();
	}
}

function updateStaticContainer(e){
	const container = document.getElementById("staticIPContainer");

	for(const el of container.querySelectorAll("input")){
		el.disabled = !e.target.checked;
	}
}

async function settingsBtnHandler(){
	if(menuManager.menu === "settings"){
		document.getElementById("btnSettings").textContent = "Settings";
		menuManager.setMenu(await updateLampState());
	}
	else{
		menuManager.setMenu("settings");
		document.getElementById("btnSettings").textContent = "Back"
	}
}

async function updateNetworkSettings(){
	document.getElementById("btnApplySettings").disabled = true;
	const createAPchecked = document.getElementById("inputCreateAP").checked;

	const data = {
		networkType: createAPchecked ? 0 : 1
	};

	if(data.networkType === 1){
		const selectedNetwork = networkManager.getSelected();

		if(selectedNetwork === null){
			data.networkSelected = false;
		}
		else{
			data.networkSelected = true;
			data.selectedNetwork = selectedNetwork;
		}
	}

	if(document.getElementById("inputStaticIP").checked){
		data.staticIP = 1;
		data.staticIPData = {
			localIP: splitIpIntoOctets(document.getElementById("inputLocalIP").value),
			gateway: splitIpIntoOctets(document.getElementById("inputGateway").value),
			subnet: splitIpIntoOctets(document.getElementById("inputSubnet").value),
		};
	}
	else{
		data.staticIP = 0;
	}

	const req = await fetch("/updateSettings", { method: "POST", body: `data=${JSON.stringify(data)}` });

	document.getElementById("btnApplySettings").disabled = false;

	if(req.status === 200){
		window.location.reload();
	}
	else{
		console.warn("Something went wrong when updating settings. STATUS: " + req.status);
	}
}

function splitIpIntoOctets(ip){
	return ip.split(".").map(oct => parseInt(oct));
}
