import QtQuick
import QtQuick.Window
import QtTest
import TransitionTest 1.0

TestCase {
    id: testCase
    name: "LoginTransition"
    when: windowShown

    TransitionBackend { id: backend }
    TransitionScreenManager { id: manager }
    property var root
    property var windows: []

    function init() {
        backend.completed = 0
        const component = Qt.createComponent("qrc:/transition-test/MainWindow.qml")
        compare(component.status, Component.Ready, component.errorString())
        root = component.createObject(testCase, {
            width: 640, height: 480, greeterBackend: backend,
            screenManager: manager
        })
        verify(root !== null)
        for (let i = 0; i < 2; ++i) {
            const win = root.screenWindow.createObject(root, {visible: true})
            windows.push(win)
        }
        manager.currentWindow = windows[0]
        wait(50)
    }
    function cleanup() {
        manager.currentWindow = null
        for (const win of windows) win.destroy()
        windows = []
        root.destroy()
        wait(20)
    }
    function test_black_all_outputs() {
        backend.PrepareTransition(false)
        compare(backend.completed, 0)
        tryCompare(backend, "completed", 1)
        compare(root.transitionProgress, 1)
        for (const win of windows) {
            waitForRendering(win.contentItem)
            const picture = grabImage(win.contentItem)
            compare(picture.red(10, 10), 0)
            compare(picture.green(320, 240), 0)
            compare(picture.blue(630, 470), 0)
        }
    }
    function test_preserve_and_failure_restore() {
        backend.PrepareTransition(true)
        tryCompare(backend, "completed", 1)
        compare(root.transitionProgress, 1)
        waitForRendering(windows[0].contentItem)
        const picture = grabImage(windows[0].contentItem)
        verify(picture.red(10, 10) > 0)
        backend.LoginFailed("Session failed")
        compare(root.transitionProgress, 0)
        compare(root.transitioning, false)
    }
    function test_failure_cancels_animation() {
        backend.PrepareTransition(false)
        wait(30)
        backend.LoginFailed("Session failed")
        wait(200)
        compare(backend.completed, 0)
        compare(root.transitionProgress, 0)
        compare(root.transitioning, false)
    }
}
