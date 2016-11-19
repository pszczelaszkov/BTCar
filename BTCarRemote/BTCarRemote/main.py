from kivy.app import App
from kivy.properties import StringProperty
from kivy.core.window import Window
from kivy.uix.screenmanager import ScreenManager, Screen
from kivy.uix.scrollview import ScrollView
from kivy.uix.button import Button
from kivy.uix.label import Label
from kivy.uix.popup import Popup
from kivy.uix.gridlayout import GridLayout
from kivy.clock import Clock
from android import activity
from bluetoothManager import BluetoothManager
#CONCEPT 2 duze pakiety w obie strony dict z lista funkcji in i out

RESULT_OK = -1
#BT PACKET TYPES
INIT_TRANSMISSION = 0x01
"""
(bytes)
WRITE:0x01
READ:0x02-power(1)
"""
STATUS_TRANSMISSION = 0x02
"""
(bytes)
WRITE:0x02-ecu(2)-steering(2)-power(1)
READ:0x02-power(1)
"""
bluetooth_manager = BluetoothManager()
activity_results_callback = {}

def activity_result(request_code, result_code, data):
    activity_results_callback[request_code](result_code)

class ControlPanel_(Screen):
    def __init__(self,bluetooth_connection):
        super(ControlPanel_,self).__init__()
        self.name = "Control Panel"
        self.bluetooth_connection = bluetooth_connection
        self.add_widget(Label(text="PANEL",font_size=50,size_hint=(1,1)))
        self.transmission_callbacks = {\
                                    STATUS_TRANSMISSION:[self.send_status,self.read_status]}
    def on_enter(self):
       print "starting communication"
       self.initialize_communication()

    def update_communication(self,delta_time):
        print "read data"
        data = bluetooth_manager.read()
        if not data:
            self.update_event.cancel()
            self.manager.current = "Connection"
            #self.manager.switch_to(self.bluetooth_connection,direction = "up")

    def initialize_communication(self):
        bluetooth_manager.send(INIT_TRANSMISSION)
        self.update_event = Clock.schedule_interval(self.update_communication, 1)

    def read_status(self):
        pass

    def send_status(self):
        bluetooth_manager.send(STATUS_TRANSMISSION)

class BluetoothConnection_(Screen):
    def __init__(self,device):
        super(BluetoothConnection_,self).__init__()
        self.name = "Connection"
        self.device = device
        self.control_panel = ControlPanel_(self)
        self.status = Label(font_size=50,size_hint=(1,1))
        self.add_widget(self.status)

    def on_enter(self):
        print "BluetoothConnection:onEnter"
        self.start_connection()

    def start_connection(self):
        print "BluetoothConnection:startConnection"
        self.status.text = "CONNECTING..."
        bluetooth_manager.connect(self.device,self.connection_result)

    def connection_result(self,result):
        print "BluetoothConnection:connection_result"
        if(result):
            self.status.text = "CONNECTED."
            self.manager.current = "Control Panel"
            #self.manager.switch_to(self.control_panel,direction = "down")
        else:
            print "2nd"
            self.start_connection()

class BluetoothDevices_(Screen):
    """Entry Screen"""

    def on_enter(self):
        self.clear_widgets()
        self.scrollview = ScrollView(size_hint=(1, None),size=(Window.width, Window.height))
        self.layout = GridLayout(cols = 1,size_hint_y=None)
        self.layout.bind(minimum_height=self.layout.setter('height'))
        self.scrollview.add_widget(self.layout)
        self.add_widget(self.scrollview)

        if(bluetooth_manager.is_enabled()):
            self.prepare_devices_list(RESULT_OK)
        else:
            #register callback,enabling BT is async
            activity_results_callback[BluetoothManager.ENABLE_BT_REQ] = lambda result: self.prepare_devices_list(result)
            bluetooth_manager.enable()
    
    def on_pre_leave(self):
        Window.rotation = 270

    def prepare_devices_list(self,bt_result):
        if(bt_result != -1):
            popup = Popup(title='BT Disabled', content=Label(text='Enable Bluetooth module.'), auto_dismiss=False)
            popup.open()
            return

        self.layout.clear_widgets()
        self.layout.add_widget(Label(text="Available Devices",font_size=20,size_hint=(1,None)))
        bluetooth_manager.start_discovery(self.add_device)

    def add_device(self,device):
        btn = Button(text=device.getName(), font_size=30, size_hint_y=None, height=100)
        btn.bind(on_release= lambda instance: self.select_device(BluetoothConnection_(device)))
        self.layout.add_widget(btn)

    def select_device(self,connection):
        self.manager.add_widget(connection)
        self.manager.add_widget(connection.control_panel)
        self.manager.current = "Connection"
        self.remove_widget(self)
       # self.manager.switch_to(screen,direction = "right")

class BTCarRemote(App):

    def on_start(self):
        activity.bind(on_activity_result=activity_result)

    def on_pause(self):
        return True

    def on_stop(self):
        bluetooth_manager.disable()

if __name__ == '__main__':
    BTCarRemote().run()