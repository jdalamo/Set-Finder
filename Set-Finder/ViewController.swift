//
//  ViewController.swift
//  Set-Finder
//
//  Created by JD del Alamo on 7/13/23.
//

import UIKit
import AVFoundation

class ViewController: UIViewController, AVCaptureVideoDataOutputSampleBufferDelegate {
   @IBOutlet weak var imageView: UIImageView!
   @IBOutlet weak var pauseButton: UIButton!

   private var captureSession: AVCaptureSession = AVCaptureSession()
   private let videoDataOutput = AVCaptureVideoDataOutput()
   private var frameProcessor = FrameProcessorWrapper()
   private var shouldCapture = true

   override func viewDidLoad()
   {
      super.viewDidLoad()

      // TODO: Update to use UIWindowScene.requestGeometryUpdate()
      let orientation = getDeviceOrientation().rawValue
      UIDevice.current.setValue(orientation, forKey: "orientation")

      pauseButton.addTarget(self, action: #selector(pauseButtonTapped), for: .touchUpInside)

      self.addCameraInput()
      self.getFrames()
      DispatchQueue.global(qos: .userInitiated).async {
         self.captureSession.startRunning() // Should be called from background thread
      }
   }

   override var supportedInterfaceOrientations: UIInterfaceOrientationMask
   {
      return .landscape
   }

   override func viewWillTransition(to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator)
   {
      super.viewWillTransition(to: size, with: coordinator)
      updateConnectionVideoOrientation()
   }

   @objc private func pauseButtonTapped(sender: UIButton)
   {
      self.shouldCapture = !self.shouldCapture

      if (self.shouldCapture) {
         sender.setImage(UIImage(systemName: "pause.circle.fill"), for: .normal)
      } else {
         sender.setImage(UIImage(systemName: "play.circle.fill"), for: .normal)
      }
   }

   private func updateConnectionVideoOrientation(inverse: Bool = true)
   {
      let orientation = getDeviceOrientation()
      var videoOrientation: AVCaptureVideoOrientation
      switch (orientation) {
      case .landscapeLeft:
         videoOrientation = inverse ? .landscapeRight : .landscapeLeft
         break
      case .landscapeRight:
         videoOrientation = inverse ? .landscapeLeft : .landscapeRight
         break
      default:
         print("Unsupported device orientation")
         return
      }

      guard let connection = self.videoDataOutput.connection(with: AVMediaType.video),
            connection.isVideoOrientationSupported else {
         return
      }

      connection.videoOrientation = videoOrientation
   }

   private func addCameraInput()
   {
      guard let device = AVCaptureDevice.DiscoverySession(
         deviceTypes: [.builtInWideAngleCamera, .builtInDualCamera, .builtInTrueDepthCamera],
         mediaType: .video,
         position: .back
      ).devices.first else {
         fatalError("No back camera detected.")
      }

      let cameraInput = try! AVCaptureDeviceInput(device: device)
      self.captureSession.addInput(cameraInput)
   }

   private func getFrames()
   {
      videoDataOutput.videoSettings = [
         (kCVPixelBufferPixelFormatTypeKey as NSString) : NSNumber(value: kCVPixelFormatType_32BGRA)] as [String: Any]
      videoDataOutput.alwaysDiscardsLateVideoFrames = true
      videoDataOutput.setSampleBufferDelegate(self, queue: DispatchQueue(label: "camera.frame.processing.queue"))
      self.captureSession.addOutput(videoDataOutput)

      updateConnectionVideoOrientation(inverse: false)
   }

   func captureOutput(
      _ output: AVCaptureOutput,
      didOutput sampleBuffer: CMSampleBuffer,
      from connection: AVCaptureConnection)
   {
      if (!self.shouldCapture) {
         return
      }

      guard let imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer) else { return }
      CVPixelBufferLockBaseAddress(imageBuffer, CVPixelBufferLockFlags.readOnly)
      let baseAddress = CVPixelBufferGetBaseAddress(imageBuffer)
      let bytesPerRow = CVPixelBufferGetBytesPerRow(imageBuffer)
      let width = CVPixelBufferGetWidth(imageBuffer)
      let height = CVPixelBufferGetHeight(imageBuffer)
      let colorSpace = CGColorSpaceCreateDeviceRGB()
      var bitmapInfo: UInt32 = CGBitmapInfo.byteOrder32Little.rawValue
      bitmapInfo |= CGImageAlphaInfo.premultipliedFirst.rawValue & CGBitmapInfo.alphaInfoMask.rawValue
      let context = CGContext(data: baseAddress, width: width, height: height, bitsPerComponent: 8, bytesPerRow: bytesPerRow, space: colorSpace, bitmapInfo: bitmapInfo)
      guard let quartzImage = context?.makeImage() else { return }
      CVPixelBufferUnlockBaseAddress(imageBuffer, CVPixelBufferLockFlags.readOnly)
      let image = UIImage(cgImage: quartzImage)

      guard let processedImage = frameProcessor?.process(image) else {
         fatalError("Problem unwrapping frameProcessor")
      }

      DispatchQueue.main.async {
         self.imageView.image = processedImage
      }
   }
}
