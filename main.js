const electron = require('electron');

// Module to control application life.
// Module to create native browser window.
const { app, BrowserWindow, dialog } = electron;

const path = require('path');
const url = require('url');
const fs = require('fs');

const Aria2Module = require('./aria2/API/build/Release/main');

// Initialize Aria2
Aria2Module.ariaInit();

global.Aria2Module = Aria2Module;
global.fs = fs;
global.dialog = dialog;

// Keep a global reference of the window object, if you don't, the window will
// be closed automatically when the JavaScript object is garbage collected.
let mainWindow;

function createWindow() {
  // Create the browser window.
  mainWindow = new BrowserWindow({ width: 1000, height: 600 });

  // and load the index.html of the app.
  mainWindow.loadURL(
    url.format({
      pathname: path.join(__dirname, 'index.html'),
      protocol: 'file:',
      slashes: true,
    })
  );

  // Open the DevTools.
  mainWindow.webContents.openDevTools();

  // // Emitted when the window is closed.
  mainWindow.on('close', e => {
    // save data and quit
    Aria2Module.stopMonitoring();
    Aria2Module.killAllSession();
    Aria2Module.ariaDeInit();
    app.quit();
  });

  mainWindow.on('closed', e => {
    // Dereference the window object, usually you would store windows
    // in an array if your app supports multi windows, this is the time
    // when you should delete the corresponding element.

    mainWindow = null;
  });
}

app.on('before-quit', () => {
  mainWindow.forceClose = true;
});

// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
// Some APIs can only be used after this event occurs.
app.on('ready', createWindow);

// Quit when all windows are closed.
app.on('window-all-closed', () => {
  // On OS X it is common for applications and their menu bar
  // to stay active until the user quits explicitly with Cmd + Q
  if (process.platform !== 'darwin') {
    app.quit();
  }
});

app.on('activate', () => {
  // On OS X it's common to re-create a window in the app when the
  // dock icon is clicked and there are no other windows open.
  if (mainWindow === null) {
    createWindow();
  }
});

// In this file you can include the rest of your app's specific main process
// code. You can also put them in separate files and require them here.
