package com.goocto.balancebot

import android.content.*
import android.hardware.usb.UsbManager
import android.support.v7.app.AppCompatActivity
import android.os.Bundle
import android.hardware.usb.UsbManager.ACTION_USB_DEVICE_ATTACHED
import android.hardware.usb.UsbManager.ACTION_USB_DEVICE_DETACHED
import android.util.Log
import android.widget.TextView


private const val TAG = "MyActivity"


class MainActivity : AppCompatActivity() {

    lateinit var mUsbManager:UsbManager
    //lateinit var connection: UsbDeviceConnection

    val mBroadcastReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context?, intent: Intent?) {
            Log.d(TAG,"received")

            when ( intent!!.action ) {
                ACTION_USB_DEVICE_ATTACHED -> showDevices()
                ACTION_USB_DEVICE_DETACHED -> showDevices()
            }
        }
    }

    fun showDevices() {
        var deviceList = mUsbManager.getDeviceList()

        var textView = findViewById(R.id.text_view) as TextView

        var Msg = ""
        for ( (k,v) in deviceList ) {
            Msg += v.manufacturerName + " : " + v.productName + "\n"
        }

        if ( Msg.length == 0 ) Msg = "No devices found"
        textView.text = Msg
        Log.d(TAG,Msg)
    }


    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        mUsbManager = getSystemService(Context.USB_SERVICE) as UsbManager

        // show currently attached devices
        showDevices()

        //var connection = mUsbManager.openDevice(deviceList.get())

    }


    override fun onResume() {
        super.onResume()

        val filter = IntentFilter() //ACTION_USB_PERMISSION)
        filter.addAction(ACTION_USB_DEVICE_ATTACHED)
        filter.addAction(ACTION_USB_DEVICE_DETACHED)
        registerReceiver(mBroadcastReceiver, filter)
        showDevices()
    }

    override fun onPause() {
        super.onPause()
        unregisterReceiver(mBroadcastReceiver)
    }




}
