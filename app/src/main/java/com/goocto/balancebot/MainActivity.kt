/**
 * Connects to an Arduino compatible device over USB and establishes communication.
 * Sends accelerometer and gyro data as it is accumulated.
 * Uses a very simple protocol. See the Arduino program file for protocol specifics.
 *
 * Built and Programmed by the LogicalOctopus
 * On Github at https://github.com/GoOcto/BalanceBot
 */

package com.goocto.balancebot

import android.content.*
import android.content.pm.ActivityInfo
import android.hardware.Sensor
import android.hardware.SensorEvent
import android.hardware.SensorEventListener
import android.hardware.SensorManager
import android.hardware.usb.*
import android.support.v7.app.AppCompatActivity
import android.os.Bundle
//import com.android.example.ACTION_USB_PERMISSION
import android.hardware.usb.UsbManager.ACTION_USB_DEVICE_ATTACHED
import android.hardware.usb.UsbManager.ACTION_USB_DEVICE_DETACHED
import android.util.Log
import android.view.View
import android.view.WindowManager
import android.widget.ScrollView
import android.widget.TextView
import com.hoho.android.usbserial.driver.UsbSerialDriver
import com.hoho.android.usbserial.util.SerialInputOutputManager
import com.hoho.android.usbserial.driver.UsbSerialProber
import com.hoho.android.usbserial.driver.UsbSerialPort
import kotlinx.android.synthetic.main.activity_main.*
import java.io.IOException
import java.nio.ByteBuffer
import java.util.concurrent.Executors


class MainActivity : AppCompatActivity(), SensorEventListener {

    private val TAG = MainActivity::class.java.simpleName

    lateinit var mUsbManager:UsbManager
    lateinit var sPort:UsbSerialPort
    private var sPortReady = false

    private var mSerialIoManager: SerialInputOutputManager? = null
    private var mExecutor = Executors.newSingleThreadExecutor()

    private var mAccel = floatArrayOf(0f,0f,0f)
    private var mGyros = floatArrayOf(0f,0f,0f)

    val sensorManager: SensorManager by lazy {
        getSystemService(Context.SENSOR_SERVICE) as SensorManager
    }

    val mBroadcastReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context?, intent: Intent?) {
            Log.d(TAG,"received")

            when ( intent!!.action ) {
                //ACTION_USB_PERMISSION -> { /* TODO */ }
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

    override fun onAccuracyChanged(sensor: Sensor?, accuracy: Int) {
    }

    override fun onSensorChanged(event: SensorEvent?) {

        when (event?.sensor?.type) {

            Sensor.TYPE_ACCELEROMETER -> {
                // event.values[0],[1],[2]
                // save this info until we read the gyros
                mAccel = event.values


            }
            Sensor.TYPE_GYROSCOPE_UNCALIBRATED -> {
                // event.values[0],[1],[2]
                // we only need the Y axis
                mGyros = event.values

                // the balance algorithm primarily needs the x axis of the gyro
                // and the angle between y and z of the accelerometer
                var xGyros = (mGyros[0] * 1000).toInt()

                // calculate an angle from these values and send that via protocol
                var dAngle = Math.atan2(mAccel[2].toDouble(),mAccel[1].toDouble()) * 180.0 / Math.PI
                var iAngle = (dAngle*100).toInt()


                var msg = String.format("A%+06d\nG%+06d",iAngle,xGyros)
                update_view.text = msg
                protocolSend(msg)

            }
        }
    }

    fun protocolSend(msg:String) {
        protocolSend(msg.toByteArray())
    }

    fun protocolSend(bytes:ByteArray) {
        val send = bytes+"\n".toByteArray()
        if ( sPortReady ) {
            try {
                // it might try to write again, before it gets the message that the device has disconnected
                sPort.write(send, 100)
                Log.d(TAG, "PROTOCOL SEND: " + send)
            }
            catch(e:Exception) {
                // don't make a big deal out of it
            }
        }
    }


    fun checkDevices() {
        val deviceList = mUsbManager.getDeviceList()
        //val textView = findViewById(R.id.text_view) as TextView
        var foundRobot = false

        var Msg = ""
        for ( (k,v) in deviceList ) {
            Msg += v.manufacturerName!!  + " : " + v.productName + "\n"

            Log.d(TAG,v.toString())

//            // these should probably be moved to the manifest
//            // they're here for now, for development
//            if ( v.vendorId==0x0403 && v.productId==0x6001 ) foundRobot = true // FTDI FT232R UART (OSEPP Micro)
//            if ( v.vendorId==0x10C4 && v.productId==0xEA60 ) foundRobot = true // CP210x UART Bridge (NodeMCU)
//            if ( v.vendorId==0x1ffb && v.productId==0x2300 ) foundRobot = true // Pololu AStar32U4
//            if ( v.vendorId==0x2341 && v.productId==0x0043 ) foundRobot = true // Arduino Uno
        }

        if ( Msg.length == 0 ) Msg = "No devices found"
        text_view.text = Msg
        Log.d(TAG,Msg)

        if ( foundRobot ) startRobotCommunication()
        else              endRobotCommunication()
    }


    fun updateReceivedData(data:ByteArray) {
        Log.d("ARDUINO SERIAL RECV",data.toString(Charsets.US_ASCII) )

        //findViewById<TextView>(R.id.data_view).append(  data.toString(Charsets.US_ASCII) )
        //findViewById<ScrollView>(R.id.scroll_view).fullScroll(View.FOCUS_DOWN)
        data_view.append(  data.toString(Charsets.US_ASCII) )
        scroll_view.fullScroll(View.FOCUS_DOWN)
    }


    fun startRobotCommunication() {

        val availableDrivers = UsbSerialProber.getDefaultProber().findAllDrivers(mUsbManager)
        if (availableDrivers.isEmpty()) {
            return
        }

        Log.d(TAG,"STARTING ROBOT BUSINESS")

        // use the first available driver.
        startIoManager(availableDrivers[0])
    }


    fun endRobotCommunication() {
        stopIoManager()
    }


    private fun startIoManager(driver: UsbSerialDriver) {

        // first establish our port and then if that works start the IOManager
        // if it fails sPortReady will be false and no writes() will be allowed from our code
        sPort = driver.ports[0]

        val connection = mUsbManager.openDevice(driver.device)
            ?: // You probably need to call UsbManager.requestPermission(driver.getDevice(), ..)
            return

        // start using the serial port
        try {
            sPort.open(connection)
            sPort.setParameters(115200, 8, UsbSerialPort.STOPBITS_1, UsbSerialPort.PARITY_NONE)


            Log.d(TAG,"DTR = "+ (if (sPort.dtr) "1" else "0" ) )
            Log.d(TAG,"RTS = "+ (if (sPort.rts) "1" else "0" ) )

            sPort.setDTR(true)

            // send our first message to our Robot
            protocolSend("READ")

        } catch (e: IOException) {
            // Deal with error.
            sPortReady = false

        } finally {
            Log.i(TAG, "Starting io manager ..")
            mSerialIoManager = SerialInputOutputManager(sPort, mListener)
            mExecutor.submit(mSerialIoManager)

            button_high.visibility = View.VISIBLE
            button_low.visibility = View.VISIBLE

            sPortReady = true
        }

    }


    private fun stopIoManager() {
        if (mSerialIoManager != null) {
            Log.i(TAG, "Stopping io manager ..")
            mSerialIoManager!!.stop()
            mSerialIoManager = null

            button_high.visibility = View.INVISIBLE
            button_low.visibility = View.INVISIBLE
        }
        if ( sPortReady ) {
            sPort.setDTR(false)
            sPort.close()
            sPortReady = false
        }
    }


    private fun hideSystemUI() {
        // for a good description of how these params affect the app
        // https://developer.android.com/training/system-ui/immersive
        val decorView = window.decorView
        decorView.systemUiVisibility = (
                View.SYSTEM_UI_FLAG_IMMERSIVE or
                        View.SYSTEM_UI_FLAG_LAYOUT_STABLE or
                        View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION or
                        View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN or
                        // Hide the nav bar and status bar
                        View.SYSTEM_UI_FLAG_HIDE_NAVIGATION or
                        View.SYSTEM_UI_FLAG_FULLSCREEN
                )
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        mUsbManager = getSystemService(Context.USB_SERVICE) as UsbManager

        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT)

        hideSystemUI()
    }


    override fun onResume() {
        super.onResume()

        val filter = IntentFilter()
        filter.addAction(ACTION_USB_DEVICE_ATTACHED)
        filter.addAction(ACTION_USB_DEVICE_DETACHED)
        registerReceiver(mBroadcastReceiver, filter)

        // uncomment the sensors when we are ready for the next step
        sensorManager.registerListener(
            this,
            sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER),
            SensorManager.SENSOR_DELAY_GAME   // UI:slow(~16fps), GAME:(~50fps), FASTEST:(~500fps)
        )
        sensorManager.registerListener(
            this,
            sensorManager.getDefaultSensor(Sensor.TYPE_GYROSCOPE_UNCALIBRATED),
            SensorManager.SENSOR_DELAY_GAME
        )

        checkDevices() // will startRobotCommunication if it finds a device
    }

    override fun onPause() {
        super.onPause()

        unregisterReceiver(mBroadcastReceiver)

        sensorManager.unregisterListener(this)

        endRobotCommunication()
    }


    fun sendHigh(view:View) {
        protocolSend("HIGH")
    }


    fun sendLow(view:View) {
        protocolSend("LOW")
    }


}
