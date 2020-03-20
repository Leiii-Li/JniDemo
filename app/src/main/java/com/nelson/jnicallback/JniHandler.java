package com.nelson.jnicallback;

import android.os.Build;
import android.os.Build.VERSION;
import android.util.Log;
import androidx.annotation.Keep;

/**
 * <pre>
 *      @author  : Nelson
 *      @since   : 2020/3/19
 *      github  : https://github.com/Nelson-KK
 *      desc    :
 * </pre>
 */
public class JniHandler {

    public static final String TAG = JniHandler.class.getSimpleName();

    /**
     * 用于Native调用Java层打印信息
     * @param msg MSG
     */
    @Keep
    private void updateStatus(String msg) {
        if (msg.toLowerCase().contains("error")) {
            Log.e(TAG, "Native Err : " + msg);
        } else {
            Log.i(TAG, "Native Msg : " + msg);
        }
    }

    /**
     * 获得编译版本
     * @return 编译版本
     */
    @Keep
    static public String getBuildVersion() {
        return VERSION.RELEASE;
    }

    /**
     * 获取运行时内存
     * @return 内存
     */
    @Keep
    public long getRuntimeMemorySize() {
        return Runtime.getRuntime().freeMemory();
    }
}
