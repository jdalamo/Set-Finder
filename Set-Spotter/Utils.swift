//
//  Utils.swift
//  Set-Spotter
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

func getUiOrientation() -> UIInterfaceOrientation
{
   // Default to .landscapeRight--home button on right side
   let interfaceOrientation: UIInterfaceOrientation? =
      getWindowScene()?.interfaceOrientation

   guard interfaceOrientation != nil else {
      print("Couldn't get UI orientation")
      return .landscapeRight
   }

   if (interfaceOrientation == .landscapeLeft) {
      return .landscapeLeft
   }

   return .landscapeRight
}
