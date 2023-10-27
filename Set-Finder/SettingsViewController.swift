//
//  SettingsViewController.swift
//  Set-Finder
//
//  Created by JD del Alamo on 10/26/23.
//

import UIKit

class SettingsViewController: UIViewController {
   @IBOutlet weak var closeButton: UIButton!
   @IBOutlet weak var skipWelcomeSwitch: UISwitch!

   override func viewDidLoad() {
      super.viewDidLoad()

      self.skipWelcomeSwitch.setOn(UserDefaults.standard.bool(forKey: SKIP_WELCOME_KEY), animated: false)

      self.closeButton.addTarget(self, action: #selector(closeButtonTapped), for: .touchUpInside)
      skipWelcomeSwitch.addTarget(self, action: #selector(skipWelcomeSwitchTapped), for: .touchUpInside)
   }

   @objc private func closeButtonTapped(sender: UIButton)
   {
      self.dismiss(animated: true, completion: nil)
   }

   @objc private func skipWelcomeSwitchTapped(sender: UISwitch)
   {
      UserDefaults.standard.set(sender.isOn, forKey: SKIP_WELCOME_KEY)
   }
}
