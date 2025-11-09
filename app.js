// === JavaScript代码文件: app.js (企业级规范版 - 使用独立分支+jsDelivr) ===

import { ESPLoader, Transport } from 'esptool-js';
import JSZip from 'jszip';

document.addEventListener('DOMContentLoaded', () => {
    const connectButton = document.getElementById('connectButton');
    const flashButton = document.getElementById('flashButton');
    const chipSelect = document.getElementById('chip-select');
    const log = document.getElementById('log');

    const FIRMWARE_FILENAME = 'xxx.zip'; // 您需要确保这个文件名是正确的

    const FIRMWARE_URL = `./${FIRMWARE_FILENAME}`; // 直接使用相对路径！

    let device, transport, esploader;

    const terminal = {
        clean() { log.innerHTML = ''; },
        writeLine(data) { log.innerHTML += data + '\n'; log.scrollTop = log.scrollHeight; },
        write(data) { log.innerHTML += data; log.scrollTop = log.scrollHeight; },
    };

    /**
     * 连接设备逻辑 (代码不变)
     */
    connectButton.addEventListener('click', async () => {
        if (device) {
            if (transport) await transport.disconnect();
            device = transport = esploader = undefined;
            connectButton.textContent = '1. 连接设备';
            flashButton.disabled = true;
            terminal.writeLine('设备已断开。');
            return;
        }
        try {
            device = await navigator.serial.requestPort({});
            transport = new Transport(device);
            const baudrate = (chipSelect.value === 'esp3c3') ? 115200 : 921600;
            esploader = new ESPLoader({ transport, baudrate, terminal });
            terminal.writeLine('正在连接设备...');
            await esploader.connect();
            connectButton.textContent = '断开连接';
            flashButton.disabled = false;
            terminal.writeLine('设备连接成功！');
            terminal.writeLine(`芯片: ${esploader.chip.CHIP_NAME}`);
        } catch (e) {
            console.error(e);
            terminal.writeLine(`错误: ${e.message}`);
            if (transport) await transport.disconnect();
            device = undefined;
        }
    });

// app.js (最终修复版 - 2024年)

/**
 * 烧录固件逻辑
 */
flashButton.addEventListener('click', async () => {
    if (!esploader) {
        terminal.writeLine('错误：设备未连接。');
        return;
    }
    
    flashButton.disabled = true;
    connectButton.disabled = true;
    terminal.clean();
    terminal.writeLine('开始烧录流程...');

    try {

        terminal.writeLine(`正在下载固件: ${FIRMWARE_FILENAME}...`);
        
        const zipResponse = await fetch(FIRMWARE_URL);
        if (!zipResponse.ok) throw new Error(`下载固件包失败 (${zipResponse.status})。`);
        const zipData = await zipResponse.arrayBuffer();
        terminal.writeLine('固件包下载完成，正在解压...');

        const zip = new JSZip();
        const loadedZip = await zip.loadAsync(zipData);
        terminal.writeLine('解压成功！');

        const manifestFile = loadedZip.file("flasher_args.json");
        if (!manifestFile) throw new Error("ZIP文件中找不到 flasher_args.json");
        const manifest = JSON.parse(await manifestFile.async("string"));
        terminal.writeLine('清单文件解析成功！');

        const filesToFlash = [];
        const fileList = manifest.flash_files;
        if (!fileList) throw new Error("清单文件格式错误：找不到 'flash_files'");

        for (const address in fileList) {
            const fileName = fileList[address];
            terminal.writeLine(` -> 正在读取 ${fileName}...`);
            const file = loadedZip.file(fileName);
            if (!file) throw new Error(`错误: 在ZIP文件中找不到固件 ${fileName}。`);
            
            // 步骤 1: 以 Uint8Array 格式读取二进制数据
            const data_uint8 = await file.async("uint8array");
            if (data_uint8.byteLength === 0) {
                throw new Error(`错误: 从 ZIP 中读取文件 ${fileName} 得到的内容为空！`);
            }
            console.log(`[调试] 文件: ${fileName}, 数据大小: ${data_uint8.byteLength} 字节`);

            // ==================== 核心修复 ====================
            // 步骤 2: 将 Uint8Array 转换为 esptool-js 所需的 "二进制字符串"
            let data_bstr = "";
            for (let i = 0; i < data_uint8.length; i++) {
                data_bstr += String.fromCharCode(data_uint8[i]);
            }
            // ======================================================

            filesToFlash.push({ data: data_bstr, address: parseInt(address) });
        }
        terminal.writeLine('所有固件文件已准备就绪！');
        
        terminal.writeLine('准备烧录到设备...');
        await esploader.writeFlash({
            fileArray: filesToFlash,
            flashSize: "keep",
            eraseAll: false,
            compress: true,
            onProgress: (bytesWritten, totalBytes) => {
                const progress = Math.round((bytesWritten / totalBytes) * 100);
                const progressBar = `[${'='.repeat(Math.round(progress / 2))}${' '.repeat(50 - Math.round(progress / 2))}]`;
                terminal.write(`\r烧录进度: ${progress}% ${progressBar}`);
            },
        });

        terminal.writeLine('\n\n烧录成功！');
        terminal.writeLine('正在断开连接以重启设备...');
        await transport.disconnect();
        terminal.writeLine('设备已重启并断开连接。');

    } catch (e) {
        console.error(e);
        terminal.writeLine(`\n\n烧录过程中发生严重错误: ${e.message}`);
    } finally {
        if (transport?.connected) {
             try { await transport.disconnect(); } catch(err) { /* ignore */ }
        }
        device = transport = esploader = undefined;
        connectButton.textContent = '1. 连接设备';
        connectButton.disabled = false;
    }
});

});
