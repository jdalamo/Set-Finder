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
   @IBOutlet weak var highlightSetsSwitch: UISwitch!
   
   override func viewDidLoad() {
      super.viewDidLoad()

      closeButton.addTarget(self, action: #selector(closeButtonTapped), for: .touchUpInside)

      skipWelcomeSwitch.setOn(UserDefaults.standard.bool(forKey: SKIP_WELCOME_KEY), animated: false)

      let showSets = getShowSetsValue()
      highlightSetsSwitch.setOn(showSets, animated: false)

      skipWelcomeSwitch.addTarget(self, action: #selector(skipWelcomeSwitchTapped), for: .touchUpInside)
      highlightSetsSwitch.addTarget(self, action: #selector(highlightSetsSwitchTapped), for: .touchUpInside)
      
   }

   private func getShowSetsValue() -> Bool
   {
      var show: Bool = true
      if let mainViewController = storyboard?.instantiateViewController(withIdentifier: "main")
         as? MainViewController {
         show = mainViewController.getShowSets()
      }

      return show
   }

   @objc private func closeButtonTapped(sender: UIButton)
   {
      self.dismiss(animated: true, completion: nil)
   }

   @objc private func skipWelcomeSwitchTapped(sender: UISwitch)
   {
      UserDefaults.standard.set(sender.isOn, forKey: SKIP_WELCOME_KEY)
   }

   @objc private func highlightSetsSwitchTapped()
   {
      if let mainViewController = storyboard?.instantiateViewController(withIdentifier: "main")
         as? MainViewController {
         mainViewController.setShowSets(show: highlightSetsSwitch.isOn)
      }
   }
}
