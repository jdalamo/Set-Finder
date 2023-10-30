//
//  SettingsViewController.swift
//  Set-Spotter
//
//  Created by JD del Alamo on 10/26/23.
//

import UIKit

protocol FrameProcessorDelegate {
   func getShowSets() -> Bool
   func setShowSets(show: Bool)
}

class SettingsViewController: UIViewController {
   @IBOutlet weak var closeButton: UIButton!
   @IBOutlet weak var skipWelcomeSwitch: UISwitch!
   @IBOutlet weak var highlightSetsSwitch: UISwitch!

   var delegate: FrameProcessorDelegate?

   override func viewDidLoad() {
      super.viewDidLoad()

      closeButton.addTarget(self, action: #selector(closeButtonTapped), for: .touchUpInside)

      skipWelcomeSwitch.setOn(UserDefaults.standard.bool(forKey: SKIP_WELCOME_KEY), animated: false)
      highlightSetsSwitch.setOn((delegate?.getShowSets())!, animated: false)

      skipWelcomeSwitch.addTarget(self, action: #selector(skipWelcomeSwitchTapped), for: .touchUpInside)
      highlightSetsSwitch.addTarget(self, action: #selector(highlightSetsSwitchTapped), for: .touchUpInside)
      
   }

   @objc private func closeButtonTapped(sender: UIButton)
   {
      self.dismiss(animated: true, completion: nil)
   }

   @objc private func skipWelcomeSwitchTapped(sender: UISwitch)
   {
      UserDefaults.standard.set(sender.isOn, forKey: SKIP_WELCOME_KEY)
   }

   @objc private func highlightSetsSwitchTapped(sender: UISwitch)
   {
      UserDefaults.standard.set(sender.isOn, forKey: HIGHLIGHT_SETS_KEY)
      delegate?.setShowSets(show: sender.isOn)
   }
}
