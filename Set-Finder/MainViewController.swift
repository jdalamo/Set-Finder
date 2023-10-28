//
//  ViewController.swift
//  Set-Finder
//
//  Created by JD del Alamo on 7/13/23.
//

import UIKit
import AVFoundation

let FRAME_PROCESSOR_MAX_THREADS = 4 // More than 4 seems to have diminishing returns

class MainViewController: UIViewController, AVCaptureVideoDataOutputSampleBufferDelegate {
   @IBOutlet weak var imageView: UIImageView!
   @IBOutlet weak var pauseButton: UIButton!
   @IBOutlet weak var shareButton: UIButton!
   @IBOutlet weak var settingsButton: UIButton!
   @IBOutlet weak var numSetsLabel: UILabel!
   
   private var captureSession: AVCaptureSession = AVCaptureSession()
   private let videoDataOutput = AVCaptureVideoDataOutput()
   private var frameProcessor = FrameProcessorWrapper(Int32(FRAME_PROCESSOR_MAX_THREADS))
   private var capturing = true

   override func viewDidLoad() {
      super.viewDidLoad()

      pauseButton.addTarget(self, action: #selector(pauseButtonTapped), for: .touchUpInside)
      shareButton.addTarget(self, action: #selector(shareButtonTapped), for: .touchUpInside)
      settingsButton.addTarget(self, action: #selector(settingsButtonTapped), for: .touchUpInside)

      self.addCameraInput()
      self.getFrames()

      // TODO: update to use UIWindowScene.requestGeometryUpdate()
      let orientation = getUiOrientation()
      UIDevice.current.setValue(orientation.rawValue, forKey: "orientation")
      updateConnectionVideoOrientation(orientation: orientation)

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
      updateConnectionVideoOrientation(orientation: getUiOrientation())
   }

   @objc private func pauseButtonTapped(sender: UIButton)
   {
      self.capturing = !self.capturing

      if (self.capturing) {
         let image = UIImage(systemName: "pause.circle.fill")?.withRenderingMode(.alwaysTemplate)
         sender.setImage(image, for: .normal)
         sender.tintColor = UIColor(red: 1, green: 0, blue: 0, alpha: 1)
      } else {
         let image = UIImage(systemName: "play.circle.fill")?.withRenderingMode(.alwaysTemplate)
         sender.setImage(image, for: .normal)
         sender.tintColor = UIColor(red: 0, green: 1, blue: 0, alpha: 1)
      }
   }

   @objc private func shareButtonTapped(sender: UIButton)
   {
      if (self.capturing) {
         // User hasn't paused live frame processing--return
         return
      }

      let image = self.imageView
      let shareImage = [ image! ]
      let activityViewController = UIActivityViewController(activityItems: shareImage, applicationActivities: nil)
      activityViewController.popoverPresentationController?.sourceView = self.view

      self.present(activityViewController, animated: true, completion: nil)
   }

   @objc private func settingsButtonTapped(sender: UIButton)
   {
      if let settingsViewController = storyboard?.instantiateViewController(withIdentifier: "settings")
         as? SettingsViewController {
         present(settingsViewController, animated: true, completion: nil)
      }
   }

   private func updateConnectionVideoOrientation(orientation: UIInterfaceOrientation)
   {
      var videoOrientation: AVCaptureVideoOrientation
      switch (orientation) {
         case .landscapeLeft:
            videoOrientation = .landscapeLeft
            break
         case .landscapeRight:
            videoOrientation = .landscapeRight
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

   private func addCameraInput() {
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

   private func getFrames() {
      videoDataOutput.videoSettings = [
         (kCVPixelBufferPixelFormatTypeKey as NSString) : NSNumber(value: kCVPixelFormatType_32BGRA)] as [String: Any]
      videoDataOutput.alwaysDiscardsLateVideoFrames = true
      videoDataOutput.setSampleBufferDelegate(self, queue: DispatchQueue(label: "camera.frame.processing.queue"))
      self.captureSession.addOutput(videoDataOutput)
   }

   func captureOutput(
      _ output: AVCaptureOutput,
      didOutput sampleBuffer: CMSampleBuffer,
      from connection: AVCaptureConnection)
   {
      if (!capturing) {
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

      let numSets = Int((frameProcessor?.getNumSetsInFrame())!)

      DispatchQueue.main.async {
         self.imageView.image = processedImage
         self.updateNumSets(numSets: numSets)
      }
   }

   private func updateNumSets(numSets: Int)
   {
      self.numSetsLabel.text = String(numSets)
   }

   func getShowSets() -> Bool
   {
      return (frameProcessor?.getShowSets())!
   }

   func setShowSets(show: Bool)
   {
      frameProcessor?.setShowSets(show)
   }
}

