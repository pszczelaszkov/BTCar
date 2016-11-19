from jnius import autoclass, cast, PythonJavaClass, java_method, jnius
from android import broadcast
from kivy.event import EventDispatcher
from kivy.properties import NumericProperty
import threading

PythonActivity = autoclass('org.kivy.android.PythonActivity')
BluetoothAdapter = autoclass("android.bluetooth.BluetoothAdapter")
BluetoothDevice = autoclass("android.bluetooth.BluetoothDevice")
BluetoothSocket = autoclass('android.bluetooth.BluetoothSocket')
Intent = autoclass('android.content.Intent')
UUID = autoclass('java.util.UUID')

class BluetoothConnectionDispatcher(threading.Thread):
    """Manages connection on separate thread allowing manager to work in non-blocking manner"""

    def __init__(self,bluetooth_adapter):
        super(BluetoothConnectionDispatcher,self).__init__(self)
        self.lock = threading.Lock()
        self.lock.acquire(False)
        self.enabled = False
        self.callback = None
        self.action = None
        self.device = None
        self.socket = None
        self.output_stream = None
        self.input_stream = None
        self.bluetooth_adapter = bluetooth_adapter

    def run(self):
        while(self.enabled):
            self.lock.acquire()
            if self.action:
                action = self.action
                callback = self.callback
                self.action = None
                self.callback = None
                action(callback)
        jnius.detach()

    def set_action(self,action,callback):
        if(not self.lock.locked):
            return False
        print "BluetoothConnectionDispatcher:set_action"
        self.action = action
        self.callback = callback
        self.lock.release()
        return True

    def set_device(self,device):
        self.device = device

    def connect(self,callback):
        if(self.socket):
            self.socket.close()

        self.bluetooth_adapter.cancelDiscovery()
        self.socket = self.device.createInsecureRfcommSocketToServiceRecord(UUID.fromString("00001101-0000-1000-8000-00805F9B34FB"))
        self.input_stream = self.socket.getInputStream()
        self.output_stream = self.socket.getOutputStream()
        print "BluetoothConnectionDispatcher:Connect"
        try:
            self.socket.connect()
            callback(True)
        except:
            callback(False)


class BluetoothManager(PythonJavaClass):
    ENABLE_BT_REQ = 0x01

    def __init__(self):
        self.bluetooth_adapter = BluetoothAdapter.getDefaultAdapter()
        self.broadcast_receiver = broadcast.BroadcastReceiver(self.broadcast_receiver,["android.bluetooth.device.action.FOUND"])
        self.connection_dispatcher = BluetoothConnectionDispatcher(self.bluetooth_adapter)

    def is_enabled(self):
        return self.bluetooth_adapter.isEnabled()

    def is_connected(self):
        return self.socket.isConnected()

    def connect(self,device,callback):
        print "BluetoothManager:Connect"
        self.connection_dispatcher.set_device(device)
        self.connection_dispatcher.set_action(self.connection_dispatcher.connect,callback)

        if(not self.connection_dispatcher.enabled):
            self.connection_dispatcher.enabled = True
            self.connection_dispatcher.start()

    def enable(self):
        current_activity = cast("android.app.Activity",PythonActivity.mActivity)
        intent = Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE)
        current_activity.startActivityForResult(intent,BluetoothManager.ENABLE_BT_REQ)

    def disable(self):
        self.bluetooth_adapter.cancelDiscovery()
        self.bluetooth_adapter.disable()
        self.broadcast_receiver.stop()
        self.connection_dispatcher.enabled = False
        if(self.connection_dispatcher.lock.locked):
            self.connection_dispatcher.lock.release()

        self.connection_dispatcher.join()

    def send(self,message):
        output_stream = self.connection_dispatcher.output_stream
        output_stream.write(message)
        output_stream.flush()

    def read(self):
        input_stream = self.connection_dispatcher.input_stream
        bytes_to_read = input_stream.available()
        
        if(not bytes_to_read):
            return None

        return [input_stream.read() for i in xrange(bytes_to_read)]

    def start_discovery(self,discovery_callback):
        self.discovery_callback = discovery_callback
        self.broadcast_receiver.start()
        self.bluetooth_adapter.startDiscovery()

    def broadcast_receiver(self,context,intent):
        device = cast("android.bluetooth.BluetoothDevice",intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE))
        self.discovery_callback(device)