//
//  Utils.swift
//  Set-Finder
//
//  Created by JD del Alamo on 9/14/23.
//

import Foundation
import UIKit

func getWindowScene() -> UIWindowScene?
{
   if #available(iOS 15, *) {
      return UIApplication.shared.connectedScenes
         .first(where: { $0 is UIWindowScene })
         .flatMap({ $0 as? UIWindowScene })
   }

   return UIApplication.shared.windows.first?.windowScene
}

func getDeviceOrientation() -> UIDeviceOrientation
{
   var orientation = UIDevice.current.orientation
   let interfaceOrientation: UIInterfaceOrientation? =
      getWindowScene()?.interfaceOrientation

   guard interfaceOrientation != nil else {
      return orientation
   }

   if (!orientation.isValidInterfaceOrientation) {
      switch interfaceOrientation {
      case .portrait:
         orientation = .portrait
         break
      case .landscapeRight:
         orientation = .landscapeRight
         break
      case .landscapeLeft:
         orientation = .landscapeLeft
         break
      case .portraitUpsideDown:
         orientation = .portraitUpsideDown
         break
      default:
         orientation = .unknown
         break
      }
   }

   return orientation
}
