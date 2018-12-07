package com.goocto.balancebot

import android.content.*
import android.hardware.usb.*
import android.support.v7.app.AppCompatActivity
import android.os.Bundle
import android.hardware.usb.UsbManager.ACTION_USB_DEVICE_ATTACHED
import android.hardware.usb.UsbManager.ACTION_USB_DEVICE_DETACHED
import android.util.Log
import android.widget.TextView
import com.hoho.android.usbserial.util.SerialInputOutputManager
import com.hoho.android.usbserial.driver.UsbSerialProber
import com.hoho.android.usbserial.driver.UsbSerialPort
import java.io.IOException
import java.util.concurrent.Executors



private const val TAG = "MyActivity"


class MainActivity : AppCompatActivity() {

    lateinit var mUsbManager:UsbManager
    lateinit var sPort:UsbSerialPort
    private var mSerialIoManager: SerialInputOutputManager? = null
    private var mExecutor = Executors.newSingleThreadExecutor()

    val mBroadcastReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context?, intent: Intent?) {
            Log.d(TAG,"received")

            when ( intent!!.action ) {
                ACTION_USB_DEVICE_ATTACHED -> checkDevices()
                ACTION_USB_DEVICE_DETACHED -> checkDevices()
            }
        }
    }

    private val mListener = object : SerialInputOutputManager.Listener {
        override fun onRunError(e: Exception) {
            Log.d(TAG, "Runner stopped.")
        }
        override fun onNewData(data: ByteArray) {
            this@MainActivity.runOnUiThread(Runnable { this@MainActivity.updateReceivedData(data) })
        }
    }


    fun checkDevices() {
        val deviceList = mUsbManager.getDeviceList()
        val textView = findViewById(R.id.text_view) as TextView
        var foundRobot = false

        var Msg = ""
        for ( (k,v) in deviceList ) {
            Msg += v.manufacturerName + " : " + v.productName + "\n"

            Log.d(TAG,v.toString())

            // VID/PID of OSEPP Micro
            if ( v.vendorId==1027 && v.productId==24577 ) {
                foundRobot = true
            }
        }

        if ( Msg.length == 0 ) Msg = "No devices found"
        textView.text = Msg
        Log.d(TAG,Msg)

        if ( foundRobot ) beginRobotBusiness()
        else              endRobotBusiness()
    }


    fun updateReceivedData(data:ByteArray) {
        Log.d(TAG,"RECEIVED DATA: " + data.toString(Charsets.US_ASCII) )
    }


    fun endRobotBusiness() {
        stopIoManager()
    }


    fun beginRobotBusiness() {
        val availableDrivers = UsbSerialProber.getDefaultProber().findAllDrivers(mUsbManager)
        if (availableDrivers.isEmpty()) {
            return
        }

        // Open a connection to the first available driver.
        val driver = availableDrivers[0]

//        /* -- instead of getting this in situ, we can add a device filter in the manifest */
//        val mPermissionIntent = PendingIntent.getBroadcast(
//            this, 0,
//            Intent(com.android.example.ACTION_USB_PERMISSION), 0
//        )
//        mUsbManager.requestPermission(driver.device, mPermissionIntent )

        val connection = mUsbManager.openDevice(driver.device)
            ?: // You probably need to call UsbManager.requestPermission(driver.getDevice(), ..)
            return

        // start using the serial port
        sPort = driver.ports[0]
        try {
            sPort.open(connection)
            sPort.setParameters(115200, 8, UsbSerialPort.STOPBITS_1, UsbSerialPort.PARITY_NONE)

            startIoManager()

            // send our first message to our new Robot
            sPort.write("READ\n".toByteArray(),100)

        } catch (e: IOException) {
            // Deal with error.
        } finally {
            //port.close()
        }
    }


    private fun stopIoManager() {
        if (mSerialIoManager != null) {
            Log.i(TAG, "Stopping io manager ..")
            mSerialIoManager!!.stop()
            mSerialIoManager = null
        }
    }


    private fun startIoManager() {
        if (sPort != null) {
            Log.i(TAG, "Starting io manager ..")
            mSerialIoManager = SerialInputOutputManager(sPort, mListener)
            mExecutor.submit(mSerialIoManager)
        }
    }


    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        mUsbManager = getSystemService(Context.USB_SERVICE) as UsbManager

        // see if any devices are already attached
        // maybe we don't need this if we registered ACTION_USB_DEVICE_ATTACHED in the manifest
        checkDevices()
    }


    override fun onResume() {
        super.onResume()

        val filter = IntentFilter() //ACTION_USB_PERMISSION)
        filter.addAction(ACTION_USB_DEVICE_ATTACHED)
        filter.addAction(ACTION_USB_DEVICE_DETACHED)
        registerReceiver(mBroadcastReceiver, filter)
        //showDevices()
    }

    override fun onPause() {
        super.onPause()
        unregisterReceiver(mBroadcastReceiver)
    }


}
